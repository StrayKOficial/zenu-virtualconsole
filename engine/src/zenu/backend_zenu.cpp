#ifdef PLATFORM_ZENU
#include "zenu.hpp"
#include "font.hpp"

namespace zenu {
    // Hardware Mappings
    volatile u32* const INPUT_REGS  = (volatile u32*)0x02000000;
    volatile f32* const ANALOG_REGS = (volatile f32*)0x02000010; // LX, LY, RX, RY
    volatile f32* const APU_REGS    = (volatile f32*)0x02000100;
    
    // GPU Registers
    volatile u32* const REG_GPU_CMD   = (volatile u32*)0x03000000;
    volatile u32* const REG_GPU_COLOR = (volatile u32*)0x03000004;
    volatile i16* const REG_GPU_X1    = (volatile i16*)0x03000008;
    volatile i16* const REG_GPU_Y1    = (volatile i16*)0x0300000A;
    volatile i16* const REG_GPU_X2    = (volatile i16*)0x0300000C;
    volatile i16* const REG_GPU_Y2    = (volatile i16*)0x0300000E;
    volatile i16* const REG_GPU_X3    = (volatile i16*)0x03000010;
    volatile i16* const REG_GPU_Y3    = (volatile i16*)0x03000012;

    void gfx_init() {
        // Zenu hardware doesn't need init
    }
    
    void gfx_begin_frame() {
        // Wait for VSync not implemented in simple emulator
    }
    
    void gfx_end_frame() {
        // Swap buffers handled by hardware
    }
    
    void gfx_clear(Color color) {
        REG_GPU_COLOR = color.to_u32();
        REG_GPU_CMD = 0; // CLEAR
    }
    
    void gfx_draw_rect(i32 x, i32 y, i32 w, i32 h, Color color) {
        // Zenu GPU doesn't have rect, use triangles
        gfx_draw_triangle(x, y, x + w, y, x, y + h, color);
        gfx_draw_triangle(x + w, y, x + w, y + h, x, y + h, color);
    }
    
    void gfx_draw_line(i32 x1, i32 y1, i32 x2, i32 y2, Color color) {
        REG_GPU_COLOR = color.to_u32();
        REG_GPU_X1 = (i16)x1; REG_GPU_Y1 = (i16)y1;
        REG_GPU_X2 = (i16)x2; REG_GPU_Y2 = (i16)y2;
        REG_GPU_CMD = 1; // DRAW_LINE
    }

    void gfx_draw_triangle(i32 x1, i32 y1, i32 x2, i32 y2, i32 x3, i32 y3, Color color) {
        REG_GPU_COLOR = color.to_u32();
        REG_GPU_X1 = (i16)x1; REG_GPU_Y1 = (i16)y1;
        REG_GPU_X2 = (i16)x2; REG_GPU_Y2 = (i16)y2;
        REG_GPU_X3 = (i16)x3; REG_GPU_Y3 = (i16)y3;
        REG_GPU_CMD = 2; // DRAW_TRIANGLE
    }

    void gfx_draw_char(i32 x, i32 y, char c, Color color) {
        if (c < 32 || c > 127) return;
        const u8* glyph = font_8x8[c - 32];
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (glyph[row] & (0x80 >> col)) {
                    gfx_draw_rect(x + col, y + row, 1, 1, color);
                }
            }
        }
    }

    void gfx_draw_text(i32 x, i32 y, const char* text, Color color) {
        int cx = x;
        while (*text) {
            if (*text == '\n') {
                y += 9;
                cx = x;
            } else {
                gfx_draw_char(cx, y, *text, color);
                cx += 8;
            }
            text++;
        }
    }

    // Input
    static u8 prev_input = 0;
    static u8 curr_input = 0;

    void update_input_state() {
        prev_input = curr_input;
        curr_input = (u8)(*INPUT_REGS); 
    }

    bool input_just_pressed(Button btn) {
        return (curr_input & btn) && !(prev_input & btn);
    }
    
    bool input_is_down(Button btn) {
        return curr_input & btn;
    }

    float input_get_axis(AnalogAxis axis) {
        if (axis < 0 || axis > 3) return 0.0f;
        return ANALOG_REGS[axis];
    }
    
    // Audio
    void audio_play_note(int channel, float freq, float volume, WaveType wave_type) {
        volatile f32* ch = APU_REGS + (channel * 4);
        ch[0] = freq;
        ch[1] = volume;
        ch[2] = (float)wave_type;
        ch[3] = 1.0f; // Trigger
    }
    
    void audio_stop(int channel) {
        volatile f32* ch = APU_REGS + (channel * 4);
        ch[1] = 0.0f; // Vol 0
    }
}

// Entry Point
extern "C" void _start() {
    zenu::gfx_init();
    ::game_init();
    
    while (true) {
        zenu::update_input_state();
        ::game_update();
        zenu::gfx_clear(zenu::Color::from_rgba(0,0,0)); // Prevent trails if user forgets
        ::game_draw();
        // VSync/Swap is implicit in hardware
    }
}
#endif
