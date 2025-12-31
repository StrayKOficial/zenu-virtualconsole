#ifndef ZENU_MATH_HPP
#define ZENU_MATH_HPP

#include "types.hpp"

namespace zenu {
    // Fixed-point 16.16
    struct Fixed {
        i32 value;

        Fixed() : value(0) {}
        Fixed(i32 v, bool raw) : value(v) {}
        Fixed(int v) : value(v << 16) {}
        Fixed(float v) : value((i32)(v * 65536.0f)) {}

        Fixed operator+(const Fixed& other) const { return Fixed(value + other.value, true); }
        Fixed operator-(const Fixed& other) const { return Fixed(value - other.value, true); }
        Fixed operator*(const Fixed& other) const {
            return Fixed((i32)(((i64)value * other.value) >> 16), true);
        }
        Fixed operator/(const Fixed& other) const {
            return Fixed((i32)(((i64)value << 16) / other.value), true);
        }

        float to_float() const { return (float)value / 65536.0f; }
        int to_int() const { return value >> 16; }
    };

    struct Vec2 {
        Fixed x, y;
        Vec2() {}
        Vec2(Fixed x, Fixed y) : x(x), y(y) {}
    };

    struct Vec3 {
        Fixed x, y, z;
        Vec3() {}
        Vec3(Fixed x, Fixed y, Fixed z) : x(x), y(y), z(z) {}
    };
}

#endif
