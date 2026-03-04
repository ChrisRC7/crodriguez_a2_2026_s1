#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cmath>
#include <thread>
#include <mutex>
#include <queue>

// --- ESTRUCTURAS MATEMÁTICAS BÁSICAS ---
struct Vec3 {
    double x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {

    }

    Vec3 operator+(const Vec3& v) const { 
        return Vec3(x + v.x, y + v.y, z + v.z); 
    }

    Vec3 operator-(const Vec3& v) const { 
        return Vec3(x - v.x, y - v.y, z - v.z); }

    Vec3 operator*(double t) const { 
        return Vec3(x * t, y * t, z * t); 
    }

    double dot(const Vec3& v) const { 
        return x * v.x + y * v.y + z * v.z; 
    }

    Vec3 normalize() const {
        double mg = std::sqrt(x*x + y*y + z*z);
        return Vec3(x/mg, y/mg, z/mg);
    }
};

struct Ray {
    Vec3 origin, direction;
    Ray(const Vec3& origin, const Vec3& direction) : origin(origin), direction(direction) {

    }
    Vec3 at(double t) const { 
        return origin + direction * t; 
    }
};

// --- LÓGICA DEL RAY TRACING ---
double hit_sphere(const Vec3& center, double radius, const Ray& r) {
    Vec3 oc = r.origin - center;
    double a = r.direction.dot(r.direction);
    double b = 2.0 * oc.dot(r.direction);
    double c = oc.dot(oc) - radius*radius;
    double discriminant = b*b - 4*a*c;
    if (discriminant < 0) return -1.0;
    return (-b - std::sqrt(discriminant)) / (2.0*a);
}

Vec3 ray_color(const Ray& r) {
    Vec3 sphere_center(0, 0, -1);
    double t = hit_sphere(sphere_center, 0.5, r);
    if (t > 0.0) {
        Vec3 N = (r.at(t) - sphere_center).normalize();
        return Vec3(N.x+1, N.y+1, N.z+1) * 0.5;
    }
    
    Vec3 unit_direction = r.direction.normalize();
    t = 0.5 * (unit_direction.y + 1.0);
    return Vec3(1.0, 1.0, 1.0) * (1.0 - t) + Vec3(0.5, 0.7, 1.0) * t;
}

// Variables globales para la cámara virtual
Vec3 origin(0, 0, 0);
Vec3 horizontal(4.0, 0, 0);
Vec3 vertical(0, 2.25, 0);
Vec3 lower_left_corner = origin - horizontal*0.5 - vertical*0.5 - Vec3(0, 0, 1);

// --- MODELOS DE EJECUCIÓN ---

// 1. Sistema Base: Secuencial Puro
void render_sequential(int width, int height, std::vector<Vec3>& fb) {
    for (int j = height - 1; j >= 0; --j) {
        for (int i = 0; i < width; ++i) {
            double u = double(i) / (width - 1);
            double v = double(j) / (height - 1);
            Ray r(origin, lower_left_corner + horizontal*u + vertical*v - origin);
            fb[(height - 1 - j) * width + i] = ray_color(r);
        }
    }
}

// Función de trabajo para un bloque de filas (usada en Grano Grueso, SMT y CMP)
void render_chunk(int start_row, int end_row, int width, int height, std::vector<Vec3>& fb) {
    for (int j = start_row; j >= end_row; --j) {
        for (int i = 0; i < width; ++i) {
            double u = double(i) / (width - 1);
            double v = double(j) / (height - 1);
            Ray r(origin, lower_left_corner + horizontal*u + vertical*v - origin);
            fb[(height - 1 - j) * width + i] = ray_color(r);
        }
    }
}

// 2. Multihilo de Grano Grueso (Coarse-Grained) / CMP / SMT Base
void render_coarse_grained(int width, int height, std::vector<Vec3>& fb, int num_threads) {
    std::vector<std::thread> threads;
    int rows_per_thread = height / num_threads;

    for (int t = 0; t < num_threads; ++t) {
        int start_row = height - 1 - (t * rows_per_thread);
        int end_row = (t == num_threads - 1) ? 0 : start_row - rows_per_thread + 1;
        
        threads.emplace_back(render_chunk, start_row, end_row, width, height, std::ref(fb));
    }

    // Esperar a que todos los bloques largos terminen (sincronización final)
    for (auto& th : threads) {
        th.join();
    }
}

// 3. Multihilo de Grano Fino (Fine-Grained - Simulación Round-Robin)
struct HardwareThreadContext {
    int start_pixel;
    int end_pixel;
    int current_pixel;
};

void render_fine_grained_simulated(int width, int height, std::vector<Vec3>& fb, int num_hw_threads, int quantum) {
    int total_pixels = width * height;
    int pixels_per_thread = total_pixels / num_hw_threads;
    
    std::vector<HardwareThreadContext> hw_threads(num_hw_threads);
    
    // Inicializar el contexto de los "hilos de hardware"
    for(int t = 0; t < num_hw_threads; ++t) {
        hw_threads[t].start_pixel = t * pixels_per_thread;
        hw_threads[t].end_pixel = (t == num_hw_threads - 1) ? total_pixels : (t + 1) * pixels_per_thread;
        hw_threads[t].current_pixel = hw_threads[t].start_pixel;
    }

    bool work_remaining = true;
    
    // Planificador simulado tipo Round-Robin
    while(work_remaining) {
        work_remaining = false;
        
        for(int t = 0; t < num_hw_threads; ++t) {
            int pixels_processed = 0;
            
            // Procesar el cuantum mínimo de rayos
            while(hw_threads[t].current_pixel < hw_threads[t].end_pixel && pixels_processed < quantum) {
                work_remaining = true;
                
                int p = hw_threads[t].current_pixel;
                int i = p % width;
                int j = height - 1 - (p / width); // Invertir Y
                
                double u = double(i) / (width - 1);
                double v = double(j) / (height - 1);
                Ray r(origin, lower_left_corner + horizontal*u + vertical*v - origin);
                
                fb[p] = ray_color(r);
                
                hw_threads[t].current_pixel++;
                pixels_processed++;
            }
            // Aquí ocurre el "Cambio de Contexto" simulado al pasar al siguiente hilo del loop
        }
    }
}


// --- FUNCIÓN PRINCIPAL ---
int main() {
    const int width = 800;
    const int height = 450;
    std::vector<Vec3> framebuffer(width * height);
    std::chrono::duration<double> duration_seq, duration_coarse, duration_fine, duration_smt;

    //std::cout << "=== Evaluando Modelos de Ejecucion (" << width << "x" << height << ") ===" << std::endl;

    // 1. Secuencial
    auto start = std::chrono::high_resolution_clock::now();
    render_sequential(width, height, framebuffer);
    duration_seq = std::chrono::high_resolution_clock::now() - start;
    //std::cout << "[1] Secuencial: " << duration_seq.count() << "s" << std::endl;

    // 2. Grano Grueso (Coarse-Grained)
    int num_threads_coarse = 2; // Aqui va los nucleos fisicos reales (2 en mi laptop o 4 en la Compu de Mesa)
    start = std::chrono::high_resolution_clock::now();
    render_coarse_grained(width, height, framebuffer, num_threads_coarse);
    duration_coarse = std::chrono::high_resolution_clock::now() - start;
    //std::cout << "[2] Grano Grueso (" << num_threads_coarse << " hilos): " << duration_coarse.count() << "s" << std::endl;

    // 3. Grano Fino (Simulación Round Robin)
    int num_simulated_threads = 4;
    int quantum = 50; // Calculan 50 rayos y cambian de contexto
    start = std::chrono::high_resolution_clock::now();
    render_fine_grained_simulated(width, height, framebuffer, num_simulated_threads, quantum);
    duration_fine = std::chrono::high_resolution_clock::now() - start;
    //std::cout << "[3] Grano Fino Simulado (Q=" << quantum << "): " << duration_fine.count() << "s" << std::endl;

    // SMT y CMP usan la misma función base que el Grano Grueso, lo que cambia es el entorno de hardware
    int num_threads_smt = num_threads_coarse * 2; 
    start = std::chrono::high_resolution_clock::now();
    render_coarse_grained(width, height, framebuffer, num_threads_smt);
    duration_smt = std::chrono::high_resolution_clock::now() - start;
    //std::cout << "[4] SMT / Sobre-suscripcion (" << num_threads_smt << " hilos): " << duration_smt.count() << "s" << std::endl;

    std::cout << duration_seq.count() << ";" 
              << duration_coarse.count() << ";" 
              << duration_fine.count() << ";" 
              << duration_smt.count() << ";"
              << std::endl;

    return 0;
}