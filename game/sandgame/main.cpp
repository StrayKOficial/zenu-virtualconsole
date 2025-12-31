#include "zenu.hpp"
#include "scene_generated.hpp"

using namespace zenu;

// --- Grid Constants ---
const int GRID_W = 160;
const int GRID_H = 144;

// --- Particle Types ---
enum ParticleType : u8 {
    EMPTY = 0,
    SAND,
    WATER,
    FIRE,
    STONE,
    SMOKE,
    OIL
};

// --- Grid State ---
u8 grid[GRID_H][GRID_W] = {0};
u8 updated[GRID_H][GRID_W] = {0}; // Prevent double-updates per frame

// --- Game State ---
enum GameState { MENU, PLAYING };
GameState state = MENU;
int cursor_x = GRID_W / 2, cursor_y = GRID_H / 2;
ParticleType brush = SAND;
int brush_size = 2;
int frames = 0;

// --- Colors ---
Color get_particle_color(ParticleType type, int x, int y) {
    int v = (x * 7 + y * 13) % 30; // Variation
    switch (type) {
        case SAND:  return Color::from_rgba(220 - v, 180 - v, 100 - v);
        case WATER: return Color::from_rgba(30, 80 + v, 200 + v/2, 180);
        case FIRE:  return Color::from_rgba(255, 100 + v * 3, v * 2);
        case STONE: return Color::from_rgba(80 + v, 80 + v, 90 + v);
        case SMOKE: return Color::from_rgba(100 + v, 100 + v, 110 + v, 120);
        case OIL:   return Color::from_rgba(50 + v, 30 + v/2, 20, 200);
        default:    return Color::from_rgba(0, 0, 0, 0);
    }
}

// --- Physics Helpers ---
bool in_bounds(int x, int y) { return x >= 0 && x < GRID_W && y >= 0 && y < GRID_H; }
bool is_empty(int x, int y) { return in_bounds(x, y) && grid[y][x] == EMPTY; }
bool is_liquid(int x, int y) { return in_bounds(x, y) && (grid[y][x] == WATER || grid[y][x] == OIL); }

void swap_cells(int x1, int y1, int x2, int y2) {
    u8 temp = grid[y1][x1];
    grid[y1][x1] = grid[y2][x2];
    grid[y2][x2] = temp;
    updated[y2][x2] = 1;
}

// --- Particle Update Logic ---
void update_sand(int x, int y) {
    if (is_empty(x, y + 1) || is_liquid(x, y + 1)) {
        swap_cells(x, y, x, y + 1);
    } else {
        int dir = (frames + x) % 2 == 0 ? -1 : 1;
        if (is_empty(x + dir, y + 1) || is_liquid(x + dir, y + 1)) {
            swap_cells(x, y, x + dir, y + 1);
        } else if (is_empty(x - dir, y + 1) || is_liquid(x - dir, y + 1)) {
            swap_cells(x, y, x - dir, y + 1);
        }
    }
}

void update_water(int x, int y) {
    if (is_empty(x, y + 1)) {
        swap_cells(x, y, x, y + 1);
    } else {
        int dir = (frames + x + y) % 2 == 0 ? -1 : 1;
        if (is_empty(x + dir, y + 1)) {
            swap_cells(x, y, x + dir, y + 1);
        } else if (is_empty(x - dir, y + 1)) {
            swap_cells(x, y, x - dir, y + 1);
        } else if (is_empty(x + dir, y)) {
            swap_cells(x, y, x + dir, y);
        } else if (is_empty(x - dir, y)) {
            swap_cells(x, y, x - dir, y);
        }
    }
}

void update_fire(int x, int y) {
    // Fire rises and has a short lifespan
    if ((frames + x * y) % 3 == 0) {
        // Chance to create smoke and die
        grid[y][x] = SMOKE;
        return;
    }
    if (is_empty(x, y - 1)) {
        swap_cells(x, y, x, y - 1);
    } else {
        int dir = (frames % 2) * 2 - 1;
        if (is_empty(x + dir, y - 1)) swap_cells(x, y, x + dir, y - 1);
        else if (is_empty(x - dir, y - 1)) swap_cells(x, y, x - dir, y - 1);
    }
    // Spread fire to adjacent oil
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (in_bounds(x + dx, y + dy) && grid[y + dy][x + dx] == OIL) {
                if ((frames + x + y) % 5 == 0) grid[y + dy][x + dx] = FIRE;
            }
        }
    }
}

void update_smoke(int x, int y) {
    if ((frames + x * y) % 8 == 0) {
        grid[y][x] = EMPTY; // Smoke dissipates
        return;
    }
    if (is_empty(x, y - 1)) {
        swap_cells(x, y, x, y - 1);
    } else {
        int dir = (frames + x) % 2 == 0 ? -1 : 1;
        if (is_empty(x + dir, y - 1)) swap_cells(x, y, x + dir, y - 1);
        else if (is_empty(x + dir, y)) swap_cells(x, y, x + dir, y);
    }
}

void update_oil(int x, int y) {
    // Oil is like water but floats on top of water and is flammable
    if (is_empty(x, y + 1)) {
        swap_cells(x, y, x, y + 1);
    } else if (in_bounds(x, y + 1) && grid[y + 1][x] == WATER) {
        // Oil floats on water - swap with water below
        swap_cells(x, y, x, y + 1);
    } else {
        int dir = (frames + x) % 2 == 0 ? -1 : 1;
        if (is_empty(x + dir, y)) swap_cells(x, y, x + dir, y);
        else if (is_empty(x - dir, y)) swap_cells(x, y, x - dir, y);
    }
}

// --- Drawing brush ---
void place_particles(int cx, int cy, ParticleType type, int size) {
    for (int dy = -size; dy <= size; dy++) {
        for (int dx = -size; dx <= size; dx++) {
            if (dx*dx + dy*dy <= size*size) {
                int nx = cx + dx, ny = cy + dy;
                if (in_bounds(nx, ny) && grid[ny][nx] == EMPTY) {
                    grid[ny][nx] = type;
                }
            }
        }
    }
}

// --- Engine Hooks ---
void game_init() {
    // Clear grid
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++) grid[y][x] = EMPTY;
}

void game_update() {
    frames++;
    
    if (state == MENU) {
        if (input_just_pressed(BUTTON_START) || input_just_pressed(BUTTON_A)) {
            state = PLAYING;
            audio_play_note(0, 660, 0.2f, WAVE_SINE);
        }
        return;
    }

    // Cursor movement
    if (input_is_down(BUTTON_UP))    cursor_y -= 2;
    if (input_is_down(BUTTON_DOWN))  cursor_y += 2;
    if (input_is_down(BUTTON_LEFT))  cursor_x -= 2;
    if (input_is_down(BUTTON_RIGHT)) cursor_x += 2;
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_x >= GRID_W) cursor_x = GRID_W - 1;
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_y >= GRID_H) cursor_y = GRID_H - 1;

    // Brush type selection
    if (input_just_pressed(BUTTON_SELECT)) {
        brush = (ParticleType)((brush % 6) + 1); // Cycle 1-6
        audio_play_note(1, 300 + brush * 50, 0.1f, WAVE_SQUARE);
    }

    // Place particles continuously while A is held
    if (input_is_down(BUTTON_A)) {
        place_particles(cursor_x, cursor_y, brush, brush_size);
    }
    // Erase with B
    if (input_is_down(BUTTON_B)) {
        for (int dy = -brush_size; dy <= brush_size; dy++)
            for (int dx = -brush_size; dx <= brush_size; dx++)
                if (in_bounds(cursor_x + dx, cursor_y + dy))
                    grid[cursor_y + dy][cursor_x + dx] = EMPTY;
    }

    // Clear updated flags
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++) updated[y][x] = 0;

    // Update particles (bottom to top, randomize horizontal order per frame)
    for (int y = GRID_H - 2; y >= 0; y--) {
        int start = (frames % 2 == 0) ? 0 : GRID_W - 1;
        int end = (frames % 2 == 0) ? GRID_W : -1;
        int step = (frames % 2 == 0) ? 1 : -1;
        
        for (int x = start; x != end; x += step) {
            if (updated[y][x]) continue;
            
            switch (grid[y][x]) {
                case SAND:  update_sand(x, y); break;
                case WATER: update_water(x, y); break;
                case FIRE:  update_fire(x, y); break;
                case SMOKE: update_smoke(x, y); break;
                case OIL:   update_oil(x, y); break;
                default: break;
            }
        }
    }

    if (frames % 10 == 0) { audio_stop(0); audio_stop(1); }
}

void game_draw() {
    gfx_clear(Color::from_rgba(8, 8, 15));

    if (state == MENU) {
        gfx_draw_text(35, 30, "SAND GAME", Color::from_rgba(220, 180, 100));
        gfx_draw_text(25, 55, "PARTICLE SIM", Color::from_rgba(100, 150, 220));
        if ((frames / 25) % 2 == 0) gfx_draw_text(35, 90, "PRESS START", Color::from_rgba(150, 150, 170));
        gfx_draw_text(15, 115, "A=PLACE B=ERASE", Color::from_rgba(100, 100, 120));
        gfx_draw_text(15, 128, "SELECT=MATERIAL", Color::from_rgba(100, 100, 120));
        return;
    }

    // Draw all particles
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            if (grid[y][x] != EMPTY) {
                Color c = get_particle_color((ParticleType)grid[y][x], x, y);
                gfx_draw_rect(x, y, 1, 1, c);
            }
        }
    }

    // Draw cursor
    Color cursorColor = get_particle_color(brush, cursor_x, cursor_y);
    for (int i = 0; i < 4; i++) {
        int ox = (i == 0) ? -brush_size - 2 : (i == 1) ? brush_size + 2 : 0;
        int oy = (i == 2) ? -brush_size - 2 : (i == 3) ? brush_size + 2 : 0;
        if (ox != 0 || oy != 0) gfx_draw_rect(cursor_x + ox, cursor_y + oy, 1, 1, Color::from_rgba(255, 255, 255));
    }
    // Crosshair center
    gfx_draw_rect(cursor_x, cursor_y, 1, 1, Color::from_rgba(255, 255, 255, 200));

    // HUD: Current material
    const char* names[] = {"", "SAND", "WATER", "FIRE", "STONE", "SMOKE", "OIL"};
    gfx_draw_text(2, 2, names[brush], cursorColor);
}
