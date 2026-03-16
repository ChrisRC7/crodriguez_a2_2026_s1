#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex print_mutex;

void print_line(const std::string& text) {
    std::lock_guard<std::mutex> lock(print_mutex);
    std::cout << text << "\n";
}

struct ThreadStats {
    int id = 0;
    int writes = 0;
    int bursts = 0;
};

void print_report(const std::string& mode, const std::vector<ThreadStats>& stats, double seconds) {
    int total_writes = 0;
    std::cout << "Modo: " << mode << "\n";
    for (const auto& s : stats) {
        total_writes += s.writes;
        std::cout << "Hilo " << s.id
                  << " escribio " << s.writes << " veces"
                  << " | rafagas: " << s.bursts << "\n";
    }
    std::cout << "Total escrituras: " << total_writes << "\n";
    std::cout << "Tiempo(s): " << seconds << "\n";
}

void run_seq_dummy(int total_writes, std::vector<ThreadStats>& stats) {
    stats[0].bursts = 1;
    for (int i = 0; i < total_writes; i++) {
        stats[0].writes++;
        print_line("[seq] Hilo 1 escribe");
    }
}

void run_parallel_dummy(int total_writes, int num_threads, std::vector<ThreadStats>& stats) {
    std::vector<std::thread> workers;
    int chunk = total_writes / num_threads;

    for (int t = 0; t < num_threads; t++) {
        int start = t * chunk;
        int end = (t == num_threads - 1) ? total_writes : start + chunk;
        workers.emplace_back([t, start, end, &stats]() {
            stats[t].bursts = 1;
            for (int i = start; i < end; i++) {
                stats[t].writes++;
                print_line("[paralelo] Hilo " + std::to_string(t + 1) + " escribe");
            }
        });
    }

    for (auto& th : workers) {
        th.join();
    }
}

void run_fine_dummy(int total_writes, int num_threads, std::vector<ThreadStats>& stats) {
    std::vector<int> cursor(num_threads, 0);
    std::vector<int> end(num_threads, 0);

    for (int t = 0; t < num_threads; t++) {
        cursor[t] = t * (total_writes / num_threads);
        end[t] = (t == num_threads - 1) ? total_writes : (t + 1) * (total_writes / num_threads);
    }

    bool active = true;
    while (active) {
        active = false;
        for (int t = 0; t < num_threads; t++) {
            if (cursor[t] >= end[t]) {
                continue;
            }
            active = true;
            stats[t].writes++;
            cursor[t]++;
            stats[t].bursts++;
            print_line("[fine] Hilo " + std::to_string(t + 1) + " escribe");
        }
    }
}

struct CoarseContext {
    int cursor = 0;
    int end = 0;
    int stall_us = 0;
    int write_window_us = 0;
    std::chrono::high_resolution_clock::time_point ready_time;
};

void run_coarse_dummy(int total_writes, int n_contexts, std::vector<ThreadStats>& stats) {
    std::vector<CoarseContext> ctx(n_contexts);

    auto now = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < n_contexts; i++) {
        ctx[i].cursor = i * (total_writes / n_contexts);
        ctx[i].end = (i == n_contexts - 1) ? total_writes : (i + 1) * (total_writes / n_contexts);
        ctx[i].stall_us = 200 + (i * 180);
        ctx[i].write_window_us = 140 + (i * 120);
        ctx[i].ready_time = now;
    }

    int next_context = 0;
    bool active = true;
    while (active) {
        active = false;
        now = std::chrono::high_resolution_clock::now();
        bool executed = false;

        for (int scan = 0; scan < n_contexts; scan++) {
            int t = (next_context + scan) % n_contexts;
            if (ctx[t].cursor >= ctx[t].end) {
                continue;
            }

            active = true;
            if (now < ctx[t].ready_time) {
                continue;
            }

            stats[t].bursts++;
            print_line(
                "[coarse] Hilo " + std::to_string(t + 1) +
                " entra. Escribe por " + std::to_string(ctx[t].write_window_us) +
                "us y luego timeout de " + std::to_string(ctx[t].stall_us) + "us"
            );

            auto slice_start = std::chrono::high_resolution_clock::now();
            while (ctx[t].cursor < ctx[t].end &&
                   (std::chrono::high_resolution_clock::now() - slice_start) < std::chrono::microseconds(ctx[t].write_window_us)) {
                stats[t].writes++;
                ctx[t].cursor++;
                print_line("[coarse] Hilo " + std::to_string(t + 1) + " escribe");
                std::this_thread::sleep_for(std::chrono::microseconds(60));
            }

            ctx[t].ready_time = std::chrono::high_resolution_clock::now() + std::chrono::microseconds(ctx[t].stall_us);
            print_line("[coarse] Hilo " + std::to_string(t + 1) + " se bloquea (timeout)");

            next_context = (t + 1) % n_contexts;
            executed = true;
            break;
        }

        if (active && !executed) {
            auto earliest = std::chrono::high_resolution_clock::time_point::max();
            for (const auto& c : ctx) {
                if (c.cursor < c.end && c.ready_time < earliest) {
                    earliest = c.ready_time;
                }
            }
            if (earliest != std::chrono::high_resolution_clock::time_point::max()) {
                std::this_thread::sleep_until(earliest);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: ./dummy_threads <modo> (seq, coarse, fine, smt, cmp)\n";
        return 1;
    }

    std::string mode = argv[1];
    const int total_writes = 12;
    const int num_threads_coarse = 2;
    const int num_threads_cmp = 2;
    const int num_threads_smt = num_threads_coarse * 2;

    std::vector<ThreadStats> stats;
    if (mode == "seq") {
        stats.resize(1);
    } else if (mode == "cmp") {
        stats.resize(num_threads_cmp);
    } else if (mode == "smt") {
        stats.resize(num_threads_smt);
    } else if (mode == "fine" || mode == "coarse") {
        stats.resize(num_threads_coarse);
    } else {
        std::cerr << "Modo invalido. Use: seq, coarse, fine, smt, cmp\n";
        return 1;
    }

    for (int i = 0; i < static_cast<int>(stats.size()); i++) {
        stats[i].id = i + 1;
    }

    auto start = std::chrono::high_resolution_clock::now();

    if (mode == "seq") {
        run_seq_dummy(total_writes, stats);
    } else if (mode == "coarse") {
        run_coarse_dummy(total_writes, num_threads_coarse, stats);
    } else if (mode == "cmp") {
        run_parallel_dummy(total_writes, num_threads_cmp, stats);
    } else if (mode == "fine") {
        run_fine_dummy(total_writes, num_threads_coarse, stats);
    } else if (mode == "smt") {
        run_parallel_dummy(total_writes, num_threads_smt, stats);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    print_report(mode, stats, duration.count());
    return 0;
}