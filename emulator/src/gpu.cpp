#include "gpu.hpp"
#include <iostream>
#include <cstring>

GPU::GPU() : window(nullptr), renderer(nullptr), texture(nullptr) {
    for (int i = 0; i < WIDTH * HEIGHT; i++) screen[i] = 0xFF000000; // Black
}

GPU::~GPU() {
    cleanup();
}

bool GPU::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Zenu Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH * 2, HEIGHT * 2, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    return true;
}

void GPU::render(uint8_t* vram) {
    uint32_t mode = *(uint32_t*)(vram + 0xFF000C);
    
    if (mode == 0) {
        // Mode 0: Direct Framebuffer
        std::memcpy(screen, vram, WIDTH * HEIGHT * sizeof(uint32_t));
    } else if (mode == 1) {
        // Mode 1: Tilemap
        int scrollX = *(int32_t*)(vram + 0xFF0004);
        int scrollY = *(int32_t*)(vram + 0xFF0008);
        uint32_t* tile_data = (uint32_t*)vram;
        uint16_t* tile_map = (uint16_t*)(vram + 0x100000);

        for (int ty = 0; ty < 30; ty++) {
            for (int tx = 0; tx < 40; tx++) {
                uint16_t tile_idx = tile_map[ty * 40 + tx];
                uint32_t* tile = tile_data + (tile_idx * 64);
                for (int py = 0; py < 8; py++) {
                    for (int px = 0; px < 8; px++) {
                        int sx = (tx * 8 + px - scrollX);
                        int sy = (ty * 8 + py - scrollY);
                        if (sx >= 0 && sx < WIDTH && sy >= 0 && sy < HEIGHT) screen[sy * WIDTH + sx] = tile[py * 8 + px];
                    }
                }
            }
        }
    } else if (mode == 2) {
        // Mode 2: Command Buffer / 3D Mode
        uint32_t cmd = *(uint32_t*)(vram + 0xFF0020);
        if (cmd != 0) {
            uint32_t color = *(uint32_t*)(vram + 0xFF0030);
            int16_t x1 = *(int16_t*)(vram + 0xFF0024);
            int16_t y1 = *(int16_t*)(vram + 0xFF0026);
            int16_t x2 = *(int16_t*)(vram + 0xFF0028);
            int16_t y2 = *(int16_t*)(vram + 0xFF002A);
            int16_t x3 = *(int16_t*)(vram + 0xFF002C);
            int16_t y3 = *(int16_t*)(vram + 0xFF002E);

            if (cmd == 1) draw_line(x1, y1, x2, y2, color);
            else if (cmd == 2) draw_triangle(x1, y1, x2, y2, x3, y3, color);

            *(uint32_t*)(vram + 0xFF0020) = 0;
        }
    }

    SDL_UpdateTexture(texture, NULL, screen, WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void GPU::cleanup() {
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void GPU::update() {
    // Placeholder for tile/sprite rendering logic
}

void GPU::draw_line(int x1, int y1, int x2, int y2, uint32_t color) {
    // Simple Bresenham's line algorithm
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        if (x1 >= 0 && x1 < WIDTH && y1 >= 0 && y1 < HEIGHT) screen[y1 * WIDTH + x1] = color;
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void GPU::draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color) {
    // Basic Scanline Triangle Rasterizer
    auto fillBottomFlatTriangle = [&](int vx1, int vy1, int vx2, int vy2, int vx3, int vy3) {
        float invslope1 = (float)(vx2 - vx1) / (vy2 - vy1);
        float invslope2 = (float)(vx3 - vx1) / (vy3 - vy1);
        float curx1 = vx1, curx2 = vx1;
        for (int scanlineY = vy1; scanlineY <= vy2; scanlineY++) {
            draw_line((int)curx1, scanlineY, (int)curx2, scanlineY, color);
            curx1 += invslope1; curx2 += invslope2;
        }
    };
    auto fillTopFlatTriangle = [&](int vx1, int vy1, int vx2, int vy2, int vx3, int vy3) {
        float invslope1 = (float)(vx3 - vx1) / (vy3 - vy1);
        float invslope2 = (float)(vx3 - vx2) / (vy3 - vy2);
        float curx1 = vx3, curx2 = vx3;
        for (int scanlineY = vy3; scanlineY > vy1; scanlineY--) {
            draw_line((int)curx1, scanlineY, (int)curx2, scanlineY, color);
            curx1 -= invslope1; curx2 -= invslope2;
        }
    };

    // Sort vertices by Y
    if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }
    if (y1 > y3) { std::swap(x1, x3); std::swap(y1, y3); }
    if (y2 > y3) { std::swap(x2, x3); std::swap(y2, y3); }

    if (y2 == y3) fillBottomFlatTriangle(x1, y1, x2, y2, x3, y3);
    else if (y1 == y2) fillTopFlatTriangle(x1, y1, x2, y2, x3, y3);
    else {
        int x4 = (int)(x1 + ((float)(y2 - y1) / (float)(y3 - y1)) * (x3 - x1));
        fillBottomFlatTriangle(x1, y1, x2, y2, x4, y2);
        fillTopFlatTriangle(x2, y2, x4, y2, x3, y3);
    }
}
