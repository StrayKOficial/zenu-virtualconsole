#include "zenu.hpp"
#include <cstdio>

using namespace zenu;

void game_init() {
}

void game_update() {
}

void game_draw() {
    gfx_clear(Color::from_rgba(10, 10, 30));
    
    gfx_draw_text(10, 10, "DUAL ANALOG TEST", Color::from_rgba(0, 255, 255));
    
    float lx = input_get_axis(AXIS_LEFT_X);
    float ly = input_get_axis(AXIS_LEFT_Y);
    float rx = input_get_axis(AXIS_RIGHT_X);
    float ry = input_get_axis(AXIS_RIGHT_Y);
    
    char buf[64];
    
    // Left Stick UI
    gfx_draw_text(10, 40, "LEFT STICK", Color::from_rgba(255, 255, 255));
    sprintf(buf, "X: %.2f Y: %.2f", lx, ly);
    gfx_draw_text(10, 50, buf, Color::from_rgba(200, 200, 200));
    gfx_draw_rect(30 + lx * 20, 80 + ly * 20, 6, 6, Color::from_rgba(0, 255, 0));
    gfx_draw_rect(32, 82, 2, 2, Color::from_rgba(255, 255, 255)); // Center dot
    
    // Right Stick UI
    gfx_draw_text(90, 40, "RIGHT STICK", Color::from_rgba(255, 255, 255));
    sprintf(buf, "X: %.2f Y: %.2f", rx, ry);
    gfx_draw_text(90, 50, buf, Color::from_rgba(200, 200, 200));
    gfx_draw_rect(110 + rx * 20, 80 + ry * 20, 6, 6, Color::from_rgba(255, 0, 0));
    gfx_draw_rect(112, 82, 2, 2, Color::from_rgba(255, 255, 255)); // Center dot
}
