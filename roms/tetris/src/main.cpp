#include "zenu.h"

// Game Constants
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define TILE_SIZE 8

// Tetromino definitions
const uint16_t tetrominoes[7][4] = {
    {0x0F00, 0x4444, 0x0F00, 0x4444}, // I
    {0x0660, 0x0660, 0x0660, 0x0660}, // O
    {0x0E40, 0x4C40, 0x4E00, 0x4640}, // T
    {0x06C0, 0x8C40, 0x06C0, 0x8C40}, // S
    {0x0C60, 0x4C80, 0x0C60, 0x4C80}, // Z
    {0x0E80, 0xC440, 0x2E00, 0x88C0}, // J
    {0x0E20, 0x44C0, 0x8E00, 0xC880}  // L
};

// Global State
uint32_t board[BOARD_HEIGHT][BOARD_WIDTH];
int cur_x = 4, cur_y = 0;
int cur_type = 0, cur_rot = 0;
uint32_t colors[] = {0, 0xFF00FFFF, 0xFFFFFF00, 0xFF800080, 0xFF00FF00, 0xFFFF0000, 0xFF0000FF, 0xFFFFA500};

void draw_block(int bx, int by, uint32_t color) {
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            zenu_draw_pixel(bx * 10 + x + 110, by * 10 + y + 20, color);
        }
    }
}

void render() {
    // Clear screen area
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 320; x++) {
            zenu_draw_pixel(x, y, 0xFF111111);
        }
    }

    // Draw Board
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            draw_block(x, y, board[y][x] ? colors[board[y][x]] : 0xFF222222);
        }
    }

    // Draw current piece
    uint16_t shape = tetrominoes[cur_type][cur_rot];
    for (int i = 0; i < 16; i++) {
        if (shape & (1 << (15 - i))) {
            int px = i % 4;
            int py = i / 4;
            draw_block(cur_x + px, cur_y + py, colors[cur_type + 1]);
        }
    }
}

bool check_collision(int nx, int ny, int nr) {
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

void lock_piece() {
    uint16_t shape = tetrominoes[cur_type][cur_rot];
    for (int i = 0; i < 16; i++) {
        if (shape & (1 << (15 - i))) {
            int px = cur_x + (i % 4);
            int py = cur_y + (i / 4);
            if (py >= 0) board[py][px] = cur_type + 1;
        }
    }
}

void clear_lines() {
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (!board[y][x]) { full = false; break; }
        }
        if (full) {
            for (int ty = y; ty > 0; ty--) {
                for (int x = 0; x < BOARD_WIDTH; x++) board[ty][x] = board[ty-1][x];
            }
            y++; // Check same line again
        }
    }
}

int main() {
    zenu_set_mode(0); // Pixel mode for simplicity for now
    
    int frame = 0;
    while (1) {
        uint8_t input = *(volatile uint8_t*)0x02000000;
        
        if (input & (1 << 2)) { // Left
            if (!check_collision(cur_x - 1, cur_y, cur_rot)) cur_x--;
        }
        if (input & (1 << 3)) { // Right
            if (!check_collision(cur_x + 1, cur_y, cur_rot)) cur_x++;
        }
        if (input & (1 << 4)) { // A (Rotate)
            int nr = (cur_rot + 1) % 4;
            if (!check_collision(cur_x, cur_y, nr)) cur_rot = nr;
        }

        if (frame % 10 == 0) {
            if (!check_collision(cur_x, cur_y + 1, cur_rot)) {
                cur_y++;
            } else {
                lock_piece();
                clear_lines();
                cur_x = 4; cur_y = 0;
                cur_type = (cur_type + 1) % 7;
                if (check_collision(cur_x, cur_y, cur_rot)) {
                    // Game Over - clear board
                    for(int i=0; i<BOARD_HEIGHT; i++) for(int j=0; j<BOARD_WIDTH; j++) board[i][j] = 0;
                }
            }
        }

        render();
        frame++;
        
        // Simple delay loop
        for(volatile int i=0; i<100000; i++);
    }
    return 0;
}
