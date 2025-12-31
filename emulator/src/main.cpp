#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
#include "cpu.hpp"
#include "bus.hpp"
#include "gpu.hpp"
#include "loader.hpp"
#include "apu.hpp"

// --- Native Tetris HLE Logic (Pocket Edition 160x144) ---
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BLOCK_SIZE 6
#define BOARD_X 50
#define BOARD_Y 12

uint32_t board[BOARD_HEIGHT][BOARD_WIDTH];
int cur_x = 4, cur_y = 0;
int cur_type = 0, cur_rot = 0;
int score = 0;
int lines_cleared = 0;

uint32_t colors[] = {
    0xFF000000,   // Empty
    0xFF00FFFF,   // I (Cyan)
    0xFFFFFF00,   // O (Yellow)
    0xFF800080,   // T (Purple)
    0xFF00FF00,   // S (Green)
    0xFFFF0000,   // Z (Red)
    0xFF0000FF,   // J (Blue)
    0xFFFFA500    // L (Orange)
};

const uint16_t tetrominoes[7][4] = {
    {0x0F00, 0x4444, 0x0F00, 0x4444}, {0x0660, 0x0660, 0x0660, 0x0660}, {0x0E40, 0x4C40, 0x4E00, 0x4640},
    {0x06C0, 0x8C40, 0x06C0, 0x8C40}, {0x0C60, 0x4C80, 0x0C60, 0x4C80}, {0x0E80, 0xC440, 0x2E00, 0x88C0},
    {0x0E20, 0x44C0, 0x8E00, 0xC880}
};


void hle_play_sound(Bus& bus, uint16_t freq, uint8_t volume, int type) {
    bus.write8(0x02000100, freq & 0xFF);
    bus.write8(0x02000101, (freq >> 8) & 0xFF);
    bus.write8(0x02000103, volume);
    bus.write8(0x02000102, 1 | (type << 1)); // Enable + Wave Type
}

void hle_draw_pixel(Bus& bus, int x, int y, uint32_t color) {
    if (x < 0 || x >= 160 || y < 0 || y >= 144) return;
    uint32_t* vram = (uint32_t*)bus.get_vram_ptr();
    vram[y * 160 + x] = color;
}

void hle_draw_block(Bus& bus, int bx, int by, uint32_t color) {
    for (int y = 0; y < BLOCK_SIZE-1; y++) {
        for (int x = 0; x < BLOCK_SIZE-1; x++) {
            hle_draw_pixel(bus, BOARD_X + bx * BLOCK_SIZE + x, BOARD_Y + by * BLOCK_SIZE + y, color);
        }
    }
}

void hle_draw_rect(Bus& bus, int x1, int y1, int w, int h, uint32_t color) {
    for(int y=y1; y<y1+h; y++) {
        for(int x=x1; x<x1+w; x++) {
            hle_draw_pixel(bus, x, y, color);
        }
    }
}

bool hle_check_collision(int nx, int ny, int nr) {
    uint16_t shape = tetrominoes[cur_type][nr];
    for (int i = 0; i < 16; i++) {
        if (shape & (1 << (15 - i))) {
            int px = nx + (i % 4);
            int py = ny + (i / 4);
            if (px < 0 || px >= BOARD_WIDTH || py >= BOARD_HEIGHT) return true;
            if (py >= 0 && board[py][px]) return true;
        }
    }
    return false;
}

void hle_tetris_step(Bus& bus, uint8_t input, int frame) {
    static int sound_timer = 0;
    if (sound_timer > 0) {
        sound_timer--;
        if (sound_timer == 0) bus.write8(0x02000102, 0); // Disable sound
    }

    static uint8_t last_input = 0;
    if ((input & (1 << 2)) && !(last_input & (1 << 2))) {
        if (!hle_check_collision(cur_x - 1, cur_y, cur_rot)) {
            cur_x--;
            hle_play_sound(bus, 440, 40, 1); sound_timer = 3; // Smooth A4 Sine
        }
    }
    if ((input & (1 << 3)) && !(last_input & (1 << 3))) {
        if (!hle_check_collision(cur_x + 1, cur_y, cur_rot)) {
            cur_x++;
            hle_play_sound(bus, 440, 40, 1); sound_timer = 3;
        }
    }
    if ((input & (1 << 4)) && !(last_input & (1 << 4))) {
        int nr = (cur_rot + 1) % 4;
        if (!hle_check_collision(cur_x, cur_y, nr)) {
            cur_rot = nr;
            hle_play_sound(bus, 523, 60, 1); sound_timer = 4; // C5 Sine
        }
    }
    if ((input & (1 << 1)) && frame % 2 == 0) if (!hle_check_collision(cur_x, cur_y + 1, cur_rot)) cur_y++;
    last_input = input;

    int speed = 20;
    if (frame % speed == 0) {
        if (!hle_check_collision(cur_x, cur_y + 1, cur_rot)) {
            cur_y++;
        } else {
            uint16_t shape = tetrominoes[cur_type][cur_rot];
            for (int i = 0; i < 16; i++) {
                if (shape & (1 << (15 - i))) {
                    int px = cur_x + (i % 4); int py = cur_y + (i / 4);
                    if (py >= 0) board[py][px] = cur_type + 1;
                }
            }
            hle_play_sound(bus, 220, 100, 1); sound_timer = 10; // Warm A3 Bass
            int bonus = 0;
            for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
                bool full = true;
                for (int x = 0; x < BOARD_WIDTH; x++) if (!board[y][x]) { full = false; break; }
                if (full) {
                    for (int ty = y; ty > 0; ty--) for (int x = 0; x < BOARD_WIDTH; x++) board[ty][x] = board[ty-1][x];
                    y++; bonus++; lines_cleared++;
                }
            }
            if (bonus > 0) {
                hle_play_sound(bus, 880, 150, 1); sound_timer = 15; // High A5 Sine
            }
            score += bonus * 100;
            cur_x = 4; cur_y = 0; cur_type = rand() % 7;
            if (hle_check_collision(cur_x, cur_y, cur_rot)) {
                for(int i=0; i<BOARD_HEIGHT; i++) for(int j=0; j<BOARD_WIDTH; j++) board[i][j] = 0;
                score = 0; lines_cleared = 0;
            }
        }
    }

    // Render
    uint32_t* vram = (uint32_t*)bus.get_vram_ptr();
    std::fill(vram, vram + (160 * 144), 0xFF1A1A2E); // Dark Purple WITH ALPHA
    hle_draw_rect(bus, BOARD_X - 2, BOARD_Y - 2, BOARD_WIDTH * BLOCK_SIZE + 4, BOARD_HEIGHT * BLOCK_SIZE + 4, 0xFF4E4E6E);
    hle_draw_rect(bus, BOARD_X, BOARD_Y, BOARD_WIDTH * BLOCK_SIZE, BOARD_HEIGHT * BLOCK_SIZE, 0xFF0F0F1F);
    for (int y = 0; y < BOARD_HEIGHT; y++) for (int x = 0; x < BOARD_WIDTH; x++) {
        if (board[y][x]) hle_draw_block(bus, x, y, colors[board[y][x]]);
    }
    uint16_t shape = tetrominoes[cur_type][cur_rot];
    for (int i = 0; i < 16; i++) if (shape & (1 << (15 - i))) hle_draw_block(bus, cur_x + (i % 4), cur_y + (i / 4), colors[cur_type + 1]);
    hle_draw_rect(bus, 5, 10, 35, 30, 0x33334D); 
    hle_draw_rect(bus, 120, 10, 35, 120, 0x33334D); 
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: ./build/zenu-emulator <path-to-game.boc>" << std::endl;
        return 0;
    }

    Bus bus;
    CPU cpu(bus);
    GPU gpu;
    APU apu;

    if (!gpu.init()) return -1;
    if (!apu.init()) return -1;
    bus.set_apu(&apu);

    std::vector<uint8_t> rom_data;
    Manifest manifest;

    if (!Loader::load_boc(argv[1], rom_data, manifest)) return -1;
    bus.load_rom(rom_data);
    gpu.set_title("Zenu Pocket - " + manifest.name);

    // Initialize VRAM/Hardware state
    bus.write32(0x03FF000C, 0); // GPU Mode 0: FB
    uint32_t* vram_ptr = (uint32_t*)bus.get_vram_ptr();
    std::fill(vram_ptr, vram_ptr + (160 * 144), 0xFF1A1A2E); // Dark Purple background

    bool running = true;
    SDL_Event e;
    uint32_t frame = 0;

    std::cout << "Zenu Pocket Mode Initialized: Loading " << manifest.name << std::endl;

    while (running) {
        while (SDL_PollEvent(&e) != 0) if (e.type == SDL_QUIT) running = false;
        
        const uint8_t* state = SDL_GetKeyboardState(NULL);
        uint8_t joy = 0;
        if (state[SDL_SCANCODE_UP])    joy |= (1 << 0);
        if (state[SDL_SCANCODE_DOWN])  joy |= (1 << 1);
        if (state[SDL_SCANCODE_LEFT])  joy |= (1 << 2);
        if (state[SDL_SCANCODE_RIGHT]) joy |= (1 << 3);
        if (state[SDL_SCANCODE_Z])     joy |= (1 << 4);
        if (state[SDL_SCANCODE_X])     joy |= (1 << 5);
        if (state[SDL_SCANCODE_RETURN]) joy |= (1 << 6);
        if (state[SDL_SCANCODE_SPACE])  joy |= (1 << 7);
        bus.write8(0x02000000, joy);

        // CPU Step (30MHz target: 500k instr per frame)
        for(int i = 0; i < 500000; i++) {
            cpu.step(false); // No debug for performance
            
            // Check for HLE Bridge (Magic Port)
            // Memory address 0x0200FFF0 is mapped to JOYPAD region in Bus.cpp
            // Actually let's check Bus::write8 to see if we need to handle it there
        }

        // Check the Magic Port in VRAM/JOY region (we use WRAM for the bridge for simplicity)
        // Let's check bit 0 of the JOY address we set for bridge
        if (bus.read8(0x0200FFF0) & 1) {
            hle_tetris_step(bus, joy, frame);
            bus.write8(0x0200FFF0, 0); // Clear trigger
        }

        gpu.update();
        gpu.render(bus.get_vram_ptr());
        frame++;
        SDL_Delay(16);
    }

    gpu.cleanup();
    apu.cleanup();
    return 0;
}
