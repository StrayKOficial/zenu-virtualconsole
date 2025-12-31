#include "zenu.hpp"
#include "scene_generated.hpp"
#include <cstdio>

using namespace zenu;

// --- Pro Palette & UI Styles ---
const Color COL_BG          = Color::from_rgba(9, 9, 28);
const Color COL_BORDER      = Color::from_rgba(75, 75, 110);
const Color COL_UI_TEXT     = Color::from_rgba(180, 180, 220);
const Color COL_UI_BOX      = Color::from_rgba(30, 30, 50);

// --- Constants ---
const int BOARD_WIDTH  = 10;
const int BOARD_HEIGHT = 20;
const int BLOCK_SIZE   = 6;

// --- Game State ---
enum GameState { MENU, PLAYING, GAMEOVER };
GameState state = MENU;

int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};
struct Point { int x, y; };

Point shapes[7][4] = {
    {{0,1}, {1,1}, {2,1}, {3,1}}, // I
    {{0,0}, {1,0}, {0,1}, {1,1}}, // O
    {{1,0}, {0,1}, {1,1}, {2,1}}, // T
    {{1,0}, {2,0}, {0,1}, {1,1}}, // S
    {{0,0}, {1,0}, {1,1}, {2,1}}, // Z
    {{0,0}, {0,1}, {1,1}, {2,1}}, // J
    {{2,0}, {0,1}, {1,1}, {2,1}}  // L
};

Color piece_colors[7] = {
    Color::from_rgba(0, 255, 255), // Cyan
    Color::from_rgba(255, 255, 0), // Yellow
    Color::from_rgba(128, 0, 128), // Purple
    Color::from_rgba(0, 255, 0),   // Green
    Color::from_rgba(255, 0, 0),   // Red
    Color::from_rgba(0, 0, 255),   // Blue
    Color::from_rgba(255, 127, 0)  // Orange
};

// --- Variables ---
int cur_type, next_types[3], hold_type = -1;
bool can_hold = true;
Point cur_pos, cur_blocks[4];
int score = 0, lines_cleared = 0, frames = 0, drop_timer = 0;
int total_seconds = 0, flash_timer = 0;

struct Star { float x, y, speed; } stars[40];

// --- Core Logic ---
void spawn_piece() {
    cur_type = next_types[0];
    next_types[0] = next_types[1];
    next_types[1] = next_types[2];
    next_types[2] = (frames + lines_cleared + 7) % 7;
    
    cur_pos = {BOARD_WIDTH / 2 - 2, 0};
    for(int i = 0; i < 4; i++) cur_blocks[i] = shapes[cur_type][i];
    can_hold = true;
}

bool check_collision(int dx, int dy, Point b[4]) {
    for(int i = 0; i < 4; i++) {
        int nx = cur_pos.x + b[i].x + dx, ny = cur_pos.y + b[i].y + dy;
        if(nx < 0 || nx >= BOARD_WIDTH || ny >= BOARD_HEIGHT) return true;
        if(ny >= 0 && board[ny][nx]) return true;
    }
    return false;
}

void rotate_piece() {
    if(cur_type == 1) return;
    Point n[4];
    for(int i = 0; i < 4; i++) {
        int ox = cur_blocks[i].x - 1, oy = cur_blocks[i].y - 1;
        n[i].x = -oy + 1; n[i].y = ox + 1;
    }
    if(!check_collision(0, 0, n)) for(int i=0; i<4; i++) cur_blocks[i] = n[i];
}

void hold_piece() {
    if(!can_hold) return;
    int prev_hold = hold_type;
    hold_type = cur_type;
    if(prev_hold == -1) spawn_piece();
    else {
        cur_type = prev_hold;
        cur_pos = {BOARD_WIDTH / 2 - 2, 0};
        for(int i = 0; i < 4; i++) cur_blocks[i] = shapes[cur_type][i];
    }
    can_hold = false;
    audio_play_note(1, 400, 0.2f, WAVE_TRIANGLE); // High ping
}

void lock_piece() {
    audio_play_note(1, 100, 0.5f, WAVE_SQUARE); // Thud
    for(int i = 0; i < 4; i++) {
        int bx = cur_pos.x + cur_blocks[i].x, by = cur_pos.y + cur_blocks[i].y;
        if(by >= 0) board[by][bx] = cur_type + 1;
    }
    int cleared = 0;
    for(int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        bool full = true;
        for(int x = 0; x < BOARD_WIDTH; x++) if(!board[y][x]) full = false;
        if(full) {
            cleared++;
            for(int ty = y; ty > 0; ty--) for(int tx = 0; tx < BOARD_WIDTH; tx++) board[ty][tx] = board[ty-1][tx];
            for(int tx = 0; tx < BOARD_WIDTH; tx++) board[0][tx] = 0;
            y++;
        }
    }
    if(cleared) {
        lines_cleared += cleared; score += cleared * 100 * cleared;
        flash_timer = 12;
        audio_play_note(0, 440 + cleared * 110, 0.5f, WAVE_SINE);
    }
    spawn_piece();
    if(check_collision(0, 0, cur_blocks)) state = GAMEOVER;
}

// --- Hooks ---
void game_init() {
    for(int i=0; i<40; i++) stars[i] = {(float)(i * 4), (float)(i * 3 % 144), 0.1f + (i%5) * 0.15f};
    for(int i=0; i<3; i++) next_types[i] = (i + 1) % 7;
    spawn_piece();
}

void game_update() {
    frames++;
    for(int i=0; i<40; i++) {
        stars[i].y += stars[i].speed;
        if(stars[i].y > 144) { stars[i].y = 0; stars[i].x = (frames + i * 10) % 160; }
    }

    if(state == MENU) {
        if(input_just_pressed(BUTTON_START) || input_just_pressed(BUTTON_A)) {
            state = PLAYING;
            audio_play_note(0, 880, 0.3f, WAVE_SINE);
        }
        return;
    }
    if(state == GAMEOVER) {
        if(input_just_pressed(BUTTON_START)) {
            for(int y=0; y<BOARD_HEIGHT; y++) for(int x=0; x<BOARD_WIDTH; x++) board[y][x] = 0;
            score = 0; lines_cleared = 0; total_seconds = 0; state = PLAYING; spawn_piece();
        }
        return;
    }

    if(frames % 60 == 0) total_seconds++;
    if(flash_timer > 0) flash_timer--;

    if(input_just_pressed(BUTTON_LEFT))  if(!check_collision(-1, 0, cur_blocks)) { cur_pos.x--; audio_play_note(1, 200, 0.1f, WAVE_SQUARE); }
    if(input_just_pressed(BUTTON_RIGHT)) if(!check_collision(1, 0, cur_blocks))  { cur_pos.x++; audio_play_note(1, 200, 0.1f, WAVE_SQUARE); }
    if(input_is_down(BUTTON_DOWN)) drop_timer += 5;
    if(input_just_pressed(BUTTON_A)) { rotate_piece(); audio_play_note(1, 300, 0.1f, WAVE_SQUARE); }
    if(input_just_pressed(BUTTON_B)) hold_piece();

    drop_timer++;
    if(drop_timer > 40) {
        if(!check_collision(0, 1, cur_blocks)) cur_pos.y++;
        else lock_piece();
        drop_timer = 0;
    }

    if(frames % 10 == 0) { audio_stop(0); audio_stop(1); }
}

void draw_block(int x, int y, Color c, bool ghost=false) {
    if(ghost) {
        gfx_draw_rect(x, y, BLOCK_SIZE-1, BLOCK_SIZE-1, Color::from_rgba(c.r, c.g, c.b, 40));
    } else {
        gfx_draw_rect(x, y, BLOCK_SIZE-1, BLOCK_SIZE-1, c);
        // Bevel / Highlight
        gfx_draw_rect(x, y, 1, BLOCK_SIZE-1, Color::from_rgba(255, 255, 255, 80));
        gfx_draw_rect(x, y, BLOCK_SIZE-1, 1, Color::from_rgba(255, 255, 255, 80));
    }
}

void game_draw() {
    gfx_clear(COL_BG);
    for(int i=0; i<40; i++) gfx_draw_rect(stars[i].x, stars[i].y, 1, 1, Color::from_rgba(255,255,255, 120));

    if(state == MENU) {
        gfx_draw_text(40, 50, "ZENU TETRIS", Color::from_rgba(255, 255, 0));
        if((frames / 30) % 2 == 0) gfx_draw_text(35, 90, "PRESS START", COL_UI_TEXT);
        gfx_draw_text(10, 130, "V2.0 ENGINE PRO", Color::from_rgba(255,255,255, 50));
        return;
    }

    int ox = 50, oy = 10;
    // Board Outline
    gfx_draw_rect(ox-2, oy-2, BOARD_WIDTH*BLOCK_SIZE+4, BOARD_HEIGHT*BLOCK_SIZE+4, COL_BORDER);
    gfx_draw_rect(ox, oy, BOARD_WIDTH*BLOCK_SIZE, BOARD_HEIGHT*BLOCK_SIZE, Color::from_rgba(5, 5, 15));

    // Left UI: HOLD
    gfx_draw_text(10, 10, "HOLD", COL_UI_TEXT);
    gfx_draw_rect(5, 20, 35, 35, COL_UI_BOX);
    gfx_draw_rect(5, 20, 35, 1, COL_BORDER);
    if(hold_type != -1) {
        for(int i=0; i<4; i++) draw_block(12+shapes[hold_type][i].x*6, 30+shapes[hold_type][i].y*6, piece_colors[hold_type]);
    }

    // Right UI: NEXT, SCORE, etc
    gfx_draw_text(115, 10, "NEXT", COL_UI_TEXT);
    gfx_draw_rect(115, 20, 40, 45, COL_UI_BOX);
    gfx_draw_rect(115, 20, 40, 1, COL_BORDER);
    for(int n=0; n<2; n++) {
        for(int i=0; i<4; i++) draw_block(122+shapes[next_types[n]][i].x*5, 30 + n*18 + shapes[next_types[n]][i].y*5, piece_colors[next_types[n]]);
    }

    char buf[24];
    gfx_draw_text(115, 75, "LINES", COL_UI_TEXT);
    sprintf(buf, "%d", lines_cleared); gfx_draw_text(115, 85, buf, Color::from_rgba(255,255,255));

    gfx_draw_text(115, 100, "TIME", COL_UI_TEXT);
    sprintf(buf, "%02d:%02d", total_seconds/60, total_seconds%60); gfx_draw_text(115, 110, buf, Color::from_rgba(255,255,255));

    gfx_draw_text(115, 122, "SCORE", COL_UI_TEXT);
    sprintf(buf, "%d", score); gfx_draw_text(115, 132, buf, Color::from_rgba(255,255,255));

    // Locked blocks
    for(int y=0; y<BOARD_HEIGHT; y++)
        for(int x=0; x<BOARD_WIDTH; x++)
            if(board[y][x]) draw_block(ox+x*BLOCK_SIZE, oy+y*BLOCK_SIZE, piece_colors[board[y][x]-1]);

    // Ghost preview
    int gy = cur_pos.y;
    while(!check_collision(0, (gy-cur_pos.y)+1, cur_blocks)) gy++;
    for(int i=0; i<4; i++) draw_block(ox+(cur_pos.x+cur_blocks[i].x)*BLOCK_SIZE, oy+(gy+cur_blocks[i].y)*BLOCK_SIZE, piece_colors[cur_type], true);

    // Active piece
    for(int i=0; i<4; i++) draw_block(ox+(cur_pos.x+cur_blocks[i].x)*BLOCK_SIZE, oy+(cur_pos.y+cur_blocks[i].y)*BLOCK_SIZE, piece_colors[cur_type]);

    if(flash_timer > 0) gfx_draw_rect(ox, oy, BOARD_WIDTH*BLOCK_SIZE, BOARD_HEIGHT*BLOCK_SIZE, Color::from_rgba(255,255,255, 150));

    if(state == GAMEOVER) {
        gfx_draw_rect(ox, oy, BOARD_WIDTH*BLOCK_SIZE, BOARD_HEIGHT*BLOCK_SIZE, Color::from_rgba(255,0,0,120));
        gfx_draw_text(55, 65, "GAME OVER", Color::from_rgba(255, 255, 255));
        gfx_draw_text(52, 85, "RETENTAR?", COL_UI_TEXT);
    }
}
