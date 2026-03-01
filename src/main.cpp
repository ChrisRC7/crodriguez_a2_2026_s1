#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cmath>

// --- ESTRUCTURAS MATEMÁTICAS BÁSICAS ---
struct Vec3 {
    double x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(double t) const { return Vec3(x * t, y * t, z * t); }
    double dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3 normalize() const {
        double mg = std::sqrt(x*x + y*y + z*z);
        return Vec3(x/mg, y/mg, z/mg);
    }
};

struct Ray {
    Vec3 origin, direction;
    Ray(const Vec3& origin, const Vec3& direction) : origin(origin), direction(direction) {}
    Vec3 at(double t) const { return origin + direction * t; }
};

// --- LÓGICA DEL RAY TRACING ---
// Verifica si un rayo intersecta con una esfera en el centro
double hit_sphere(const Vec3& center, double radius, const Ray& r) {
    Vec3 oc = r.origin - center;
    double a = r.direction.dot(r.direction);
    double b = 2.0 * oc.dot(r.direction);
    double c = oc.dot(oc) - radius*radius;
    double discriminant = b*b - 4*a*c;
    
    if (discriminant < 0) {
        return -1.0; // No hay intersección
    } else {
        return (-b - std::sqrt(discriminant)) / (2.0*a); // Retorna la distancia al punto de impacto
    }
}

// Calcula el color del píxel para un rayo dado
Vec3 ray_color(const Ray& r) {
    Vec3 sphere_center(0, 0, -1);
    double t = hit_sphere(sphere_center, 0.5, r);
    
    if (t > 0.0) {
        // Si golpea la esfera, calcula un color basado en la normal de la superficie
        Vec3 N = (r.at(t) - sphere_center).normalize();
        return Vec3(N.x+1, N.y+1, N.z+1) * 0.5;
    }
    
    // Si no golpea nada, dibuja un fondo degradado (cielo)
    Vec3 unit_direction = r.direction.normalize();
    t = 0.5 * (unit_direction.y + 1.0);
    return Vec3(1.0, 1.0, 1.0) * (1.0 - t) + Vec3(0.5, 0.7, 1.0) * t;
}

// --- MODELOS DE EJECUCIÓN ---

// 1. Sistema Base: Secuencial Puro
void render_sequential(int image_width, int image_height, std::vector<Vec3>& framebuffer) {
    Vec3 origin(0, 0, 0);
    Vec3 horizontal(4.0, 0, 0);
    Vec3 vertical(0, 2.25, 0);
    Vec3 lower_left_corner = origin - horizontal*0.5 - vertical*0.5 - Vec3(0, 0, 1);

    for (int j = image_height - 1; j >= 0; --j) {
        for (int i = 0; i < image_width; ++i) {
            double u = double(i) / (image_width - 1);
            double v = double(j) / (image_height - 1);
            Ray r(origin, lower_left_corner + horizontal*u + vertical*v - origin);
            
            // El cálculo pesado
            framebuffer[(image_height - 1 - j) * image_width + i] = ray_color(r);
        }
    }
}

// --- FUNCIÓN PRINCIPAL ---
int main() {
    // Configuración de la imagen
    const int image_width = 800;
    const int image_height = 450;
    std::vector<Vec3> framebuffer(image_width * image_height);

    std::cout << "=== Framework Experimental de Ejecución ===" << std::endl;
    std::cout << "Iniciando renderizado secuencial (" << image_width << "x" << image_height << ")..." << std::endl;

    // Medición de tiempo (Línea Base)
    auto start_time = std::chrono::high_resolution_clock::now();
    
    render_sequential(image_width, image_height, framebuffer);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;

    std::cout << "Renderizado Secuencial completado en: " << duration.count() << " segundos." << std::endl;

    // Guardar la imagen en formato PPM
    std::ofstream out("output_secuencial.ppm");
    out << "P3\n" << image_width << " " << image_height << "\n255\n";
    for (int i = 0; i < image_width * image_height; ++i) {
        int ir = static_cast<int>(255.999 * framebuffer[i].x);
        int ig = static_cast<int>(255.999 * framebuffer[i].y);
        int ib = static_cast<int>(255.999 * framebuffer[i].z);
        out << ir << " " << ig << " " << ib << "\n";
    }
    out.close();
    std::cout << "Imagen guardada como 'output_secuencial.ppm'." << std::endl;

    return 0;
}