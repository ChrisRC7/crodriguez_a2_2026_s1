#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <thread>
#include <random>

// --- CONFIGURACIÓN DEL STALL ARTIFICIAL (Sugerencia del Profesor) ---
const int MIN_RAYS_BEFORE_STALL = 200;
const int MAX_RAYS_BEFORE_STALL = 800;
const int MIN_STALL_US = 10;   // Microsegundos de latencia
const int MAX_STALL_US = 50;

struct Vec3 {
    double x, y, z;
    Vec3() : x(0), y(0), z(0) {

    }

    Vec3(double x, double y, double z) : x(x), y(y), z(z) {

    }
    Vec3 operator+(const Vec3& v) const { 
        return Vec3(x + v.x, y + v.y, z + v.z); 
    }

    Vec3 operator-(const Vec3& v) const { 
        return Vec3(x - v.x, y - v.y, z - v.z); 
    }

    Vec3 operator*(double t) const { 
        return Vec3(x * t, y * t, z * t); 
    }

    double dot(const Vec3& v) const { 
        return x * v.x + y * v.y + z * v.z; 
    }

    Vec3 normalize() const {
        double m = std::sqrt(x*x + y*y + z*z);
        if (m == 0.0) {
            // Avoid division by zero when normalizing a zero-length vector.
            return Vec3(0.0, 0.0, 0.0);
        }
        return Vec3(x/m, y/m, z/m);
    }
};

struct Ray {
    Vec3 origin, direction;
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d) {

    }
    Vec3 at(double t) const { 
        return origin + direction * t; 
    }
};

Vec3 ray_color(const Ray& r) {
    Vec3 sphere_center(0, 0, -1);
    Vec3 oc = r.origin - sphere_center;
    double a = r.direction.dot(r.direction);
    double b = 2.0 * oc.dot(r.direction);
    double c = oc.dot(oc) - 0.25;
    double discriminant = b*b - 4*a*c;
    

    if (discriminant > 0) {
        // Compute the nearest intersection point and use its normal for shading.
        double t = (-b - std::sqrt(discriminant)) / (2.0 * a);
        Vec3 hit_point = r.at(t);
        double radius = 0.5;
        Vec3 normal = (hit_point - sphere_center) * (1.0 / radius);
        return Vec3(0.5 * (normal.x + 1.0), 0.5 * (normal.y + 1.0), 0.5 * (normal.z + 1.0));
    }
    Vec3 unit_direction = r.direction.normalize();
    double t = 0.5 * (unit_direction.y + 1.0);
    return Vec3(1.0, 1.0, 1.0) * (1.0 - t) + Vec3(0.5, 0.7, 1.0) * t;
}

// Variables de cámara
Vec3 origin(0, 0, 0);
Vec3 horizontal(4.0, 0, 0);
Vec3 vertical(0, 2.25, 0);
Vec3 lower_left_corner = origin - horizontal*0.5 - vertical*0.5 - Vec3(0, 0, 1);


// --- 1. SECUENCIAL PURO ---
void render_sequential(int w, int h, std::vector<Vec3>& fb) {

    std::random_device rd;
    std::mt19937 rng(rd());

    std::uniform_int_distribution<int> dist_rays(MIN_RAYS_BEFORE_STALL, MAX_RAYS_BEFORE_STALL);
    std::uniform_int_distribution<int> dist_stall(MIN_STALL_US, MAX_STALL_US);

    int rays_to_stall = dist_rays(rng);
     
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            Ray r(origin, lower_left_corner + horizontal*(double(i)/w) + vertical*(double(j)/h) - origin);
            fb[j * w + i] = ray_color(r);

            rays_to_stall--;
            if (rays_to_stall <= 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(dist_stall(rng)));
                rays_to_stall = dist_rays(rng);
            }
        }
    }
}

// --- 2. EJECUCION PARALELA REAL (CMP / SMT) ---
void render_chunk_with_stalls(int start, int end, int w, int h, std::vector<Vec3>& fb) {
    // Generador de números aleatorios único para cada hilo
    thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist_rays(MIN_RAYS_BEFORE_STALL, MAX_RAYS_BEFORE_STALL);
    std::uniform_int_distribution<int> dist_stall(MIN_STALL_US, MAX_STALL_US);
    
    int rays_to_stall = dist_rays(rng);

    for (int p = start; p < end; p++) {
        int i = p % w;
        int j = p / w;
        Ray r(origin, lower_left_corner + horizontal*(double(i)/w) + vertical*(double(j)/h) - origin);
        fb[p] = ray_color(r);

        // Lógica de interrupción del profesor
        rays_to_stall--;
        if (rays_to_stall <= 0) {
            // El hilo se bloquea completamente (Blocked Multithreading)
            std::this_thread::sleep_for(std::chrono::microseconds(dist_stall(rng)));
            rays_to_stall = dist_rays(rng);
        }
    }
}

void run_parallel_chunks(int w, int h, std::vector<Vec3>& fb, int num_threads) {
    std::vector<std::thread> threads;
    int chunk = (w * h) / num_threads;
    for (int t = 0; t < num_threads; t++) {
        int start = t * chunk;
        int end = (t == num_threads - 1) ? (w * h) : start + chunk;
        threads.emplace_back(render_chunk_with_stalls, start, end, w, h, std::ref(fb));
    }
    for (auto& th : threads) th.join();
}

// --- 3. GRANO GRUESO SIMULADO (Cambio solo en bloqueos costosos) ---
struct CoarseContext {
    int cursor, end;
    int rays_to_stall;
    std::chrono::high_resolution_clock::time_point ready_time;
};

void run_coarse_grained_mt(int w, int h, std::vector<Vec3>& fb, int n_contexts) {
    int total = w * h;
    std::vector<CoarseContext> ctx(n_contexts);

    std::random_device rd;
    std::vector<std::mt19937> rngs;
    rngs.reserve(n_contexts);
    for (int i = 0; i < n_contexts; i++) {
        rngs.emplace_back(rd());
    }
    std::uniform_int_distribution<int> dist_rays(MIN_RAYS_BEFORE_STALL, MAX_RAYS_BEFORE_STALL);
    std::uniform_int_distribution<int> dist_stall(MIN_STALL_US, MAX_STALL_US);

    auto now = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < n_contexts; i++) {
        ctx[i].cursor = i * (total / n_contexts);
        ctx[i].end = (i == n_contexts - 1) ? total : (i + 1) * (total / n_contexts);
        ctx[i].rays_to_stall = dist_rays(rngs[i]);
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

            // En grano grueso, un contexto corre hasta bloquearse o terminar.
            while (ctx[t].cursor < ctx[t].end) {
                int p = ctx[t].cursor;
                int i = p % w;
                int j = p / w;
                Ray r(origin, lower_left_corner + horizontal * (double(i) / w) + vertical * (double(j) / h) - origin);
                fb[p] = ray_color(r);

                ctx[t].cursor++;
                ctx[t].rays_to_stall--;

                if (ctx[t].rays_to_stall <= 0) {
                    ctx[t].ready_time = std::chrono::high_resolution_clock::now() + std::chrono::microseconds(dist_stall(rngs[t]));
                    ctx[t].rays_to_stall = dist_rays(rngs[t]);
                    break;
                }
            }

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

// --- 4. GRANO FINO SIMULADO (Ocultando el Stall Artificial) ---
struct HWThread {
    int cursor, end;
    int rays_to_stall;
    std::chrono::high_resolution_clock::time_point ready_time; // Timer de interrupción
};

void run_fine_grained(int w, int h, std::vector<Vec3>& fb, int n_threads, int quantum) {
    int total = w * h;
    std::vector<HWThread> hwt(n_threads);
    
    std::random_device rd;
    std::vector<std::mt19937> rngs;
    for (int i = 0; i < n_threads; i++) {
        rngs.emplace_back(rd());
    }
    std::uniform_int_distribution<int> dist_rays(MIN_RAYS_BEFORE_STALL, MAX_RAYS_BEFORE_STALL);
    std::uniform_int_distribution<int> dist_stall(MIN_STALL_US, MAX_STALL_US);

    for(int i=0; i<n_threads; i++) {
        hwt[i].cursor = i * (total/n_threads);
        hwt[i].end = (i == n_threads - 1) ? total : (i+1) * (total/n_threads);
        hwt[i].rays_to_stall = dist_rays(rngs[i]);
        hwt[i].ready_time = std::chrono::high_resolution_clock::now();
    }

    bool active = true;
    while(active) {
        active = false;
        auto now = std::chrono::high_resolution_clock::now();
        
        for(int t=0; t<n_threads; t++) {
            if (hwt[t].cursor >= hwt[t].end) continue;
            active = true;

            // ¡AQUÍ ESTÁ LA MAGIA DEL GRANO FINO!
            // Si el timer del stall de este hilo no ha expirado, lo ignoramos y pasamos al siguiente.
            // Esto oculta la latencia manteniendo la CPU ocupada.
            if (now < hwt[t].ready_time) continue; 

            int processed = 0;
            while(hwt[t].cursor < hwt[t].end && processed < quantum) {
                int p = hwt[t].cursor;
                int i = p % w; 
                int j = p / w;
                Ray r(origin, lower_left_corner + horizontal*(double(i)/w) + vertical*(double(j)/h) - origin);
                fb[p] = ray_color(r);
                
                hwt[t].cursor++;
                processed++;
                hwt[t].rays_to_stall--;

                // Si ocurre la interrupción (stall)
                if (hwt[t].rays_to_stall <= 0) {
                    // Seteamos el timer para el futuro y ROMPEMOS el quantum para ceder la CPU a otro hilo
                    hwt[t].ready_time = std::chrono::high_resolution_clock::now() + std::chrono::microseconds(dist_stall(rngs[t]));
                    hwt[t].rays_to_stall = dist_rays(rngs[t]);
                    break;
                }
            }
        }
    }
}

// --- FUNCIÓN PRINCIPAL ---
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: ./raytracer <modo> (seq, coarse, fine, smt, cmp)\n";
        return 1;
    }
    std::string mode = argv[1];
    const int w = 800, h = 450;
    std::vector<Vec3> fb(w * h);

    int num_threads_coarse = 2;
    int num_threads_cmp = 2;
    int num_threads_smt = num_threads_coarse * 2; 
    int quantum = 50;

    auto start = std::chrono::high_resolution_clock::now();

    if (mode == "seq") {
        render_sequential(w, h, fb);
    } else if (mode == "coarse") {
        run_coarse_grained_mt(w, h, fb, num_threads_coarse);
    } else if (mode == "cmp") {
        run_parallel_chunks(w, h, fb, num_threads_cmp);
    } else if (mode == "fine") {
        run_fine_grained(w, h, fb, num_threads_coarse, quantum);
    } else if (mode == "smt") {
        run_parallel_chunks(w, h, fb, num_threads_smt);
    } else {
        return 1;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << duration.count() << std::endl;
    return 0;
}