#include "zenu.hpp"
#include <cmath>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <cstdlib>

using namespace zenu;

const int W = 800;
const int H = 600;
const float PI = 3.1415926535f;

// --- Float Math for 3D ---
struct V3 { float x, y, z; };
struct V2 { float x, y; };

struct Player {
    V3 pos = {0, 1.5f, 0};
    float yaw = 0;
    float pitch = 0;
} player;

// --- Scene Objects ---
struct Grass {
    V3 pos;
    float phase;
};
std::vector<Grass> grass_patches;

// --- Time State ---
float time_f = 0;

// --- 3D Projection ---
V2 project(V3 p) {
    // Relative to player
    float dx = p.x - player.pos.x;
    float dy = p.y - player.pos.y;
    float dz = p.z - player.pos.z;

    // Rotate by Yaw (Left/Right)
    float rx = dx * cosf(-player.yaw) - dz * sinf(-player.yaw);
    float rz = dx * sinf(-player.yaw) + dz * cosf(-player.yaw);

    // Rotate by Pitch (Up/Down)
    float py = dy * cosf(-player.pitch) - rz * sinf(-player.pitch);
    float rz_final = dy * sinf(-player.pitch) + rz * cosf(-player.pitch);

    if (rz_final <= 0.1f) return {-1, -1}; // Culled

    float fov = 500.0f; // Zoom factor
    float screen_x = (rx / rz_final) * fov + W / 2;
    float screen_y = (py / rz_final) * fov + H / 2;

    return {screen_x, screen_y};
}

void game_init() {
    // Generate grass randomly
    for (int i = 0; i < 400; i++) {
        float x = (float)(rand() % 2000 - 1000) / 10.0f;
        float z = (float)(rand() % 2000 - 1000) / 10.0f;
        grass_patches.push_back({{x, 0, z}, (float)(rand() % 100) / 10.0f});
    }
}

void game_update() {
    time_f += 0.016f;

    // Movement axes
    float lx = input_get_axis(AXIS_LEFT_X);
    float ly = input_get_axis(AXIS_LEFT_Y);
    float rx = input_get_axis(AXIS_RIGHT_X);
    float ry = input_get_axis(AXIS_RIGHT_Y);

    player.yaw += rx * 0.05f;
    player.pitch -= ry * 0.03f;
    if (player.pitch > 1.4f) player.pitch = 1.4f;
    if (player.pitch < -1.4f) player.pitch = -1.4f;

    float speed = 0.2f;
    // Standard WASD/Analog movement logic
    player.pos.x += (sinf(player.yaw) * ly + cosf(player.yaw) * lx) * speed;
    player.pos.z += (cosf(player.yaw) * ly - sinf(player.yaw) * lx) * speed;
}

void draw_sky() {
    // Atmospheric Scattering Sky Gradient
    for (int y = 0; y < H / 2; y++) {
        float t = (float)y / (H / 2);
        u8 r = (u8)(80 + t * 40);
        u8 g = (u8)(120 + t * 60);
        u8 b = (u8)(200 + t * 55);
        gfx_draw_rect(0, y, W, 1, Color::from_rgba(r, g, b));
    }

    // Realistic Sun with Glow
    V3 sun_pos = {50, 40, 100};
    V2 s = project(sun_pos);
    if (s.x > 0 && s.x < W && s.y > 0 && s.y < H) {
        for (int r = 60; r > 0; r -= 3) {
            u8 alpha = (u8)(120 * (1.0f - (float)r/60.0f));
            gfx_draw_rect(s.x - r, s.y - r, r*2, r*2, Color::from_rgba(255, 255, 150, alpha));
        }
        gfx_draw_rect(s.x - 12, s.y - 12, 24, 24, Color::from_rgba(255, 255, 230));
    }

    // Drifting Procedural Clouds
    for (int i = 0; i < 12; i++) {
        float cx = fmod(i * 180 + time_f * 15, W + 300) - 150;
        float cy = 80 + sinf(time_f * 0.3f + i) * 30;
        float cw = 120 + cosf(i) * 60;
        gfx_draw_rect(cx, cy, cw, 40, Color::from_rgba(255, 255, 255, 80));
    }
}

void game_draw() {
    draw_sky();

    // Terrain Plane (Coded Texture: Dark Olive with depth fog)
    gfx_draw_rect(0, H / 2, W, H / 2, Color::from_rgba(25, 45, 25));

    // Animated Grass (Billboarding + Wind)
    for (auto& gp : grass_patches) {
        // Distance check for performance and fog
        float dx = gp.pos.x - player.pos.x;
        float dz = gp.pos.z - player.pos.z;
        float dist_sq = dx*dx + dz*dz;
        if (dist_sq > 600) continue; 

        V2 base = project(gp.pos);
        if (base.x < -40 || base.x > W + 40 || base.y < -40 || base.y > H + 40) continue;

        // Visual distance
        float dist = sqrtf(dist_sq);
        float scale = 40.0f / (dist + 1.0f);
        if (scale > 30) scale = 30;

        // Wind Sway
        float wind = sinf(time_f * 2.5f + gp.pos.x * 0.5f) * 8.0f * (scale / 10.0f);
        
        // Blade Rendering
        Color grass_col = Color::from_rgba(40, (u8)(140 + wind * 2), 40);
        gfx_draw_line(base.x, base.y, base.x + wind, base.y - 20 * scale, grass_col);
        gfx_draw_line(base.x - 3, base.y, base.x + wind - 4, base.y - 15 * scale, grass_col);
        gfx_draw_line(base.x + 3, base.y, base.x + wind + 4, base.y - 15 * scale, grass_col);
    }

    // Depth Fog Overlay
    for (int y = H / 2; y < H; y++) {
        float f = 1.0f - (float)(y - H/2) / (H/4);
        if (f > 0) {
            gfx_draw_rect(0, y, W, 1, Color::from_rgba(100, 130, 160, (u8)(f * 120)));
        }
    }

    // Pro HUD
    gfx_draw_text(30,30, "ZENU HORIZON: APEX", Color::from_rgba(255, 255, 255));
    gfx_draw_text(30,48, "800x600 Software 3D Renderer", Color::from_rgba(0, 255, 200, 180));
    
    // FPS counter placeholder (can be added if needed)
}
