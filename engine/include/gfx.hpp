#ifndef ZENU_GFX_HPP
#define ZENU_GFX_HPP

#include "types.hpp"

namespace zenu {
    void gfx_init();
    void gfx_begin_frame();
    void gfx_clear(Color color);
    void gfx_draw_rect(i32 x, i32 y, i32 w, i32 h, Color color);
    void gfx_draw_line(i32 x1, i32 y1, i32 x2, i32 y2, Color color);
    void gfx_draw_triangle(i32 x1, i32 y1, i32 x2, i32 y2, i32 x3, i32 y3, Color color);
    void gfx_draw_char(i32 x, i32 y, char c, Color color);
    void gfx_draw_text(i32 x, i32 y, const char* text, Color color);
    void gfx_draw_sprite(i32 x, i32 y, i32 w, i32 h, const u32* data);
    void gfx_end_frame();
}

#endif
