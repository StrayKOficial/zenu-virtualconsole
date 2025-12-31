#ifndef ZENU_TYPES_HPP
#define ZENU_TYPES_HPP

#include <cstdint>

namespace zenu {
    typedef uint8_t u8;
    typedef uint16_t u16;
    typedef uint32_t u32;
    typedef int8_t i8;
    typedef int16_t i16;
    typedef int32_t i32;
    typedef int64_t i64;

    struct Color {
        u8 a, r, g, b;
        
        static Color from_rgba(u8 r, u8 g, u8 b, u8 a = 255) {
            return {a, r, g, b};
        }

        u32 to_u32() const {
            return (a << 24) | (r << 16) | (g << 8) | b;
        }
    };
}

#endif
