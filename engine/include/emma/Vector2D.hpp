#ifndef VECTOR2D_HPP
#define VECTOR2D_HPP

#include <cmath>

namespace emma {

// Header-only 2D vector in double precision
struct Vector2D {
    double x = 0.0;
    double y = 0.0;

    Vector2D() = default;
    Vector2D(double x_val, double y_val) : x(x_val), y(y_val) {}

    Vector2D operator+(const Vector2D& o) const { return Vector2D(x + o.x, y + o.y); }
    Vector2D operator-(const Vector2D& o) const { return Vector2D(x - o.x, y - o.y); }
    Vector2D operator-() const { return Vector2D(-x, -y); }
    Vector2D operator*(double s) const { return Vector2D(x * s, y * s); }
    Vector2D operator/(double s) const { return Vector2D(x / s, y / s); }

    Vector2D& operator+=(const Vector2D& o) {
        x += o.x;
        y += o.y;
        return *this;
    }
    Vector2D& operator-=(const Vector2D& o) {
        x -= o.x;
        y -= o.y;
        return *this;
    }

    double dot(const Vector2D& o) const { return x * o.x + y * o.y; }
    double cross(const Vector2D& o) const { return x * o.y - y * o.x; }  // z of the 3D cross product
    double lengthSquared() const { return x * x + y * y; }
    double magnitude() const { return std::sqrt(lengthSquared()); }
    Vector2D normalized() const {
        const double m = magnitude();
        return m > 0.0 ? *this / m : Vector2D();
    }
};

} // namespace emma

#endif
