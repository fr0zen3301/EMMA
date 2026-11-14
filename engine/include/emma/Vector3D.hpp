#ifndef VECTOR3D_HPP
#define VECTOR3D_HPP

#include <cmath>

namespace emma {

// Header-only 3D vector in double precision
struct Vector3D {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Vector3D() = default;
    Vector3D(double x_val, double y_val, double z_val = 0.0) : x(x_val), y(y_val), z(z_val) {}

    Vector3D operator+(const Vector3D& o) const { return Vector3D(x + o.x, y + o.y, z + o.z); }
    Vector3D operator-(const Vector3D& o) const { return Vector3D(x - o.x, y - o.y, z - o.z); }
    Vector3D operator-() const { return Vector3D(-x, -y, -z); }
    Vector3D operator*(double s) const { return Vector3D(x * s, y * s, z * s); }
    Vector3D operator/(double s) const { return Vector3D(x / s, y / s, z / s); }

    Vector3D& operator+=(const Vector3D& o) {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
    Vector3D& operator-=(const Vector3D& o) {
        x -= o.x;
        y -= o.y;
        z -= o.z;
        return *this;
    }

    double dot(const Vector3D& o) const { return x * o.x + y * o.y + z * o.z; }
    Vector3D cross(const Vector3D& o) const {
        return Vector3D(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x);
    }
    double lengthSquared() const { return x * x + y * y + z * z; }
    double magnitude() const { return std::sqrt(lengthSquared()); }
    Vector3D normalized() const {
        const double m = magnitude();
        return m > 0.0 ? *this / m : Vector3D();
    }
};

} // namespace emma

#endif
