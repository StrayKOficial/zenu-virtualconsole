#include "zenu.hpp"
#include "scene_generated.hpp"
#include <cstdio>

using namespace zenu;

// --- Pro Palette ---
const Color COL_BG      = Color::from_rgba(5, 8, 18);
const Color COL_COURT   = Color::from_rgba(15, 20, 35);
const Color COL_LINE    = Color::from_rgba(40, 50, 80);
const Color COL_PADDLE  = Color::from_rgba(0, 230, 255);
const Color COL_BALL    = Color::from_rgba(255, 255, 255);
const Color COL_SCORE   = Color::from_rgba(255, 255, 255);
const Color COL_UI      = Color::from_rgba(120, 130, 160);

// --- Game State ---
enum GameState { MENU, PLAYING, PAUSED, GAMEOVER };
GameState state = MENU;

// --- Constants ---
const int SCREEN_W = 160;
const int SCREEN_H = 144;
const int PADDLE_W = 4;
const int PADDLE_H = 24;
const int BALL_SIZE = 4;
const int WINNING_SCORE = 7;

// --- Variables ---
float player_y = SCREEN_H / 2 - PADDLE_H / 2;
float ai_y = SCREEN_H / 2 - PADDLE_H / 2;
float ball_x = SCREEN_W / 2, ball_y = SCREEN_H / 2;
float ball_vx = 1.5f, ball_vy = 0.8f;
int player_score = 0, ai_score = 0;
int frames = 0;
int flash_timer = 0;
float ai_speed = 1.2f;

struct Star { float x, y, speed; } stars[25];

// --- Core Logic ---
void reset_ball(int dir) {
    ball_x = SCREEN_W / 2;
    ball_y = SCREEN_H / 2;
    ball_vx = 1.5f * dir;
    ball_vy = ((frames % 10) - 5) * 0.2f;
    flash_timer = 8;
}

void game_init() {
    for (int i = 0; i < 25; i++) stars[i] = { (float)(i * 7 % SCREEN_W), (float)(i * 6 % SCREEN_H), 0.1f + (i % 4) * 0.1f };
}

void game_update() {
    frames++;
    for (int i = 0; i < 25; i++) {
        stars[i].x -= stars[i].speed;
        if (stars[i].x < 0) { stars[i].x = SCREEN_W; stars[i].y = (frames * 3 + i * 7) % SCREEN_H; }
    }

    if (state == MENU) {
        if (input_just_pressed(BUTTON_START) || input_just_pressed(BUTTON_A)) {
            state = PLAYING;
            player_score = 0; ai_score = 0;
            reset_ball(1);
            audio_play_note(0, 660, 0.3f, WAVE_SINE);
        }
        return;
    }

    if (state == GAMEOVER) {
        if (input_just_pressed(BUTTON_START)) {
            state = MENU;
        }
        return;
    }

    if (flash_timer > 0) flash_timer--;

    // Player Input
    if (input_is_down(BUTTON_UP))   player_y -= 2.5f;
    if (input_is_down(BUTTON_DOWN)) player_y += 2.5f;
    if (player_y < 0) player_y = 0;
    if (player_y > SCREEN_H - PADDLE_H) player_y = SCREEN_H - PADDLE_H;

    // AI Logic
    float ai_target = ball_y - PADDLE_H / 2 + (ball_vy * 3);
    if (ai_y < ai_target - 2) ai_y += ai_speed;
    else if (ai_y > ai_target + 2) ai_y -= ai_speed;
    if (ai_y < 0) ai_y = 0;
    if (ai_y > SCREEN_H - PADDLE_H) ai_y = SCREEN_H - PADDLE_H;

    // Ball Movement
    ball_x += ball_vx;
    ball_y += ball_vy;

    // Top/Bottom Walls
    if (ball_y <= 0 || ball_y >= SCREEN_H - BALL_SIZE) {
        ball_vy = -ball_vy;
        audio_play_note(1, 330, 0.1f, WAVE_SQUARE);
    }

    // Player Paddle Collision (Left)
    if (ball_x <= 10 + PADDLE_W && ball_y + BALL_SIZE >= player_y && ball_y <= player_y + PADDLE_H) {
        ball_vx = -ball_vx * 1.05f;
        ball_vy += (ball_y - (player_y + PADDLE_H / 2)) * 0.1f;
        audio_play_note(0, 550, 0.2f, WAVE_TRIANGLE);
    }

    // AI Paddle Collision (Right)
    if (ball_x >= SCREEN_W - 10 - PADDLE_W - BALL_SIZE && ball_y + BALL_SIZE >= ai_y && ball_y <= ai_y + PADDLE_H) {
        ball_vx = -ball_vx * 1.05f;
        ball_vy += (ball_y - (ai_y + PADDLE_H / 2)) * 0.1f;
        audio_play_note(0, 550, 0.2f, WAVE_TRIANGLE);
    }

    // Scoring
    if (ball_x < 0) {
        ai_score++;
        audio_play_note(0, 220, 0.5f, WAVE_SQUARE);
        reset_ball(1);
    }
    if (ball_x > SCREEN_W) {
        player_score++;
        audio_play_note(0, 880, 0.5f, WAVE_SINE);
        reset_ball(-1);
    }

    // Win Condition
    if (player_score >= WINNING_SCORE || ai_score >= WINNING_SCORE) {
        state = GAMEOVER;
    }

    // Speed up AI slightly as game progresses
    ai_speed = 1.2f + (player_score + ai_score) * 0.08f;
    
    // Clamp ball velocity
    if (ball_vx > 3.5f) ball_vx = 3.5f;
    if (ball_vx < -3.5f) ball_vx = -3.5f;

    if (frames % 8 == 0) { audio_stop(0); audio_stop(1); }
}

void game_draw() {
    gfx_clear(COL_BG);
    for (int i = 0; i < 25; i++) gfx_draw_rect(stars[i].x, stars[i].y, 1, 1, Color::from_rgba(255, 255, 255, 60));

    if (state == MENU) {
        gfx_draw_text(50, 40, "ZENU PONG", Color::from_rgba(0, 230, 255));
        if ((frames / 25) % 2 == 0) gfx_draw_text(35, 80, "PRESS START", COL_UI);
        gfx_draw_text(25, 120, "FIRST TO 7 WINS", Color::from_rgba(100, 100, 130));
        return;
    }

    // Court Background
    gfx_draw_rect(5, 5, SCREEN_W - 10, SCREEN_H - 10, COL_COURT);

    // Center Line (dashed)
    for (int y = 8; y < SCREEN_H - 8; y += 10) {
        gfx_draw_rect(SCREEN_W / 2 - 1, y, 2, 6, COL_LINE);
    }

    // Paddles
    gfx_draw_rect(10, (int)player_y, PADDLE_W, PADDLE_H, COL_PADDLE);
    gfx_draw_rect(SCREEN_W - 10 - PADDLE_W, (int)ai_y, PADDLE_W, PADDLE_H, COL_PADDLE);
    // Paddle highlights
    gfx_draw_rect(10, (int)player_y, 1, PADDLE_H, Color::from_rgba(255, 255, 255, 100));
    gfx_draw_rect(SCREEN_W - 10 - PADDLE_W, (int)ai_y, 1, PADDLE_H, Color::from_rgba(255, 255, 255, 100));

    // Ball
    gfx_draw_rect((int)ball_x, (int)ball_y, BALL_SIZE, BALL_SIZE, COL_BALL);
    // Ball glow
    gfx_draw_rect((int)ball_x - 1, (int)ball_y - 1, BALL_SIZE + 2, BALL_SIZE + 2, Color::from_rgba(255, 255, 255, 30));

    // Scores
    char buf[8];
    sprintf(buf, "%d", player_score);
    gfx_draw_text(SCREEN_W / 4 - 4, 12, buf, COL_SCORE);
    sprintf(buf, "%d", ai_score);
    gfx_draw_text(SCREEN_W * 3 / 4 - 4, 12, buf, COL_SCORE);

    // Flash effect on score
    if (flash_timer > 0) {
        gfx_draw_rect(0, 0, SCREEN_W, SCREEN_H, Color::from_rgba(255, 255, 255, 80));
    }

    if (state == GAMEOVER) {
        gfx_draw_rect(30, 50, 100, 40, Color::from_rgba(0, 0, 0, 200));
        if (player_score >= WINNING_SCORE) {
            gfx_draw_text(45, 60, "YOU WIN!", Color::from_rgba(0, 255, 100));
        } else {
            gfx_draw_text(45, 60, "YOU LOSE", Color::from_rgba(255, 80, 80));
        }
        gfx_draw_text(40, 78, "PRESS START", COL_UI);
    }
}
