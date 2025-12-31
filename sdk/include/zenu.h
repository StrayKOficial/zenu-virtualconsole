#ifndef ZENU_H
#define ZENU_H

#include <stdint.h>

// Memory Mapped I/O
#define VRAM_BASE   0x03000000
#define TILE_MAP    0x03100000
#define GPU_CTRL    0x03FF0000

#define GPU_REG_SCROLL_X (GPU_CTRL + 0x04)
#define GPU_REG_SCROLL_Y (GPU_CTRL + 0x08)
#define GPU_REG_MODE     (GPU_CTRL + 0x0C)

// Helper to write to VRAM
inline void zenu_set_mode(uint32_t mode) {
    *(volatile uint32_t*)GPU_REG_MODE = mode;
}

inline void zenu_draw_pixel(int x, int y, uint32_t color) {
    uint32_t* vram = (uint32_t*)VRAM_BASE;
    vram[y * 320 + x] = color;
}

#endif
