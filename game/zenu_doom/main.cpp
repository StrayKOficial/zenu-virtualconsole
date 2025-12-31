#include "zenu.hpp"
#include <cmath>
#include <cstdio>

using namespace zenu;

// --- Constants ---
const int MAP_W = 16;
const int MAP_H = 16;
const int SCREEN_W = 160;
const int SCREEN_H = 144;
const float PI = 3.1415926535f;

// --- Map (1 = Wall) ---
const int map[] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,1,1,0,0,0,1,1,1,0,0,0,0,1,
    1,0,0,1,1,0,0,0,1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,1,0,0,0,0,1,1,1,
    1,0,0,0,0,0,0,0,1,1,1,0,0,1,0,1,
    1,0,0,1,1,1,1,0,0,0,0,0,0,1,0,1,
    1,0,0,1,0,0,1,0,0,0,0,0,0,1,0,1,
    1,0,0,1,0,0,1,0,1,1,1,1,1,1,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,1,1,1,1,1,0,0,1,0,0,1,0,1,
    1,0,0,0,0,0,1,0,0,0,1,1,1,1,0,1,
    1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

// --- Player State ---
float player_x = 2.0f, player_y = 2.0f;
float player_angle = 1.0f;
float player_fov = PI / 3.0f; // 60 degrees

// --- Game Variables ---
int frames = 0;
int shoot_timer = 0;

void game_init() {}

void game_update() {
    frames++;
    if (shoot_timer > 0) shoot_timer--;

    // --- Modern Dual Analog Controls ---
    float lx = input_get_axis(AXIS_LEFT_X);
    float ly = input_get_axis(AXIS_LEFT_Y);
    float rx = input_get_axis(AXIS_RIGHT_X);

    // Rotation (Right Stick)
    player_angle += rx * 0.08f;

    // Movement (Left Stick forward/backward + strafe)
    float speed = 0.12f;
    float nx = player_x + (cosf(player_angle) * -ly + sinf(player_angle) * lx) * speed;
    float ny = player_y + (sinf(player_angle) * -ly - cosf(player_angle) * lx) * speed;

    // Basic Collision
    if (map[(int)ny * MAP_W + (int)nx] == 0) {
        player_x = nx;
        player_y = ny;
    }

    // Shooting
    if (input_just_pressed(BUTTON_A) && shoot_timer == 0) {
        shoot_timer = 15;
        audio_play_note(0, 150, 0.4f, WAVE_SQUARE); // Gunshot sound
    }

    if (frames % 8 == 0) audio_stop(0);
}

void game_draw() {
    // --- Render Ceiling & Floor ---
    gfx_clear(Color::from_rgba(20, 20, 25)); // Ceiling (Dark Blue)
    gfx_draw_rect(0, SCREEN_H / 2, SCREEN_W, SCREEN_H / 2, Color::from_rgba(15, 15, 15)); // Floor (Blackish)

    // --- Raycasting Loop (160 columns) ---
    for (int x = 0; x < SCREEN_W; x++) {
        // Calculate ray angle based on FOV
        float ray_angle = (player_angle - player_fov / 2.0f) + ((float)x / (float)SCREEN_W) * player_fov;

        float distance = 0.0f;
        bool hit_wall = false;
        
        float eye_x = cosf(ray_angle);
        float eye_y = sinf(ray_angle);

        while (!hit_wall && distance < 16.0f) {
            distance += 0.05f;
            int test_x = (int)(player_x + eye_x * distance);
            int test_y = (int)(player_y + eye_y * distance);

            if (test_x < 0 || test_x >= MAP_W || test_y < 0 || test_y >= MAP_H) {
                hit_wall = true;
                distance = 16.0f;
            } else if (map[test_y * MAP_W + test_x] == 1) {
                hit_wall = true;
            }
        }

        // Correct for fish-eye effect
        float corrected_dist = distance * cosf(ray_angle - player_angle);

        // Wall Height proportional to distance
        int ceiling = (int)((float)SCREEN_H / 2.0f - (float)SCREEN_H / corrected_dist);
        int floor = SCREEN_H - ceiling;
        int wall_h = floor - ceiling;

        // --- Distance Shading ---
        // Darken walls based on corrected_dist (0 to 16)
        u8 shade = (u8)(180.0f / (1.0f + corrected_dist * 0.5f));
        if (shade < 10) shade = 10;
        
        // Wall Color with simple lighting (fake shadows on certain axes)
        Color wall_col = Color::from_rgba(shade, shade, shade + 10);
        
        // Draw the vertical wall slice
        gfx_draw_rect(x, ceiling, 1, wall_h, wall_col);
    }

    // --- Weapon Sprite (Simple Rectangles) ---
    int wx = SCREEN_W / 2 - 10;
    int wy = SCREEN_H - 30 + (cosf(frames * 0.2f) * 2); // Swaying effect
    if (shoot_timer > 10) wy -= 5; // Recoil

    // Gun body
    gfx_draw_rect(wx + 4, wy, 12, 30, Color::from_rgba(40, 40, 45));
    // Barrel
    gfx_draw_rect(wx + 8, wy - 5, 4, 15, Color::from_rgba(20, 20, 22));
    
    // Muzzle Flash
    if (shoot_timer > 12) {
        gfx_draw_rect(wx + 6, wy - 10, 8, 8, Color::from_rgba(255, 200, 50, 150));
        gfx_draw_rect(wx + 8, wy - 8, 4, 4, Color::from_rgba(255, 255, 200));
    }

    // --- HUD ---
    gfx_draw_text(5, 5, "ZENU DOOM", Color::from_rgba(0, 255, 255, 100));
}
