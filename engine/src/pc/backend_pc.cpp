#ifdef PLATFORM_PC
#include "zenu.hpp"
#include "font.hpp"
#include <SDL2/SDL.h>
#include <iostream>

// --- SDL2 State ---
struct PCContext {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_AudioDeviceID audioDevice;
    SDL_GameController* controller;
    bool running;
};
static PCContext pc;

// --- APU Emulator ---
struct APUState {
    float channels[4]; // Current sample value
    float phase[4];    // Current phase
    float freq[4];     // Frequency
    float vol[4];      // Volume
    zenu::u8 wave[4];        // Wave type
    int duration[4];   // Duration in frames
};
static APUState apu;

void audio_callback(void* userdata, Uint8* stream, int len) {
    float* buffer = (float*)stream;
    int samples = len / sizeof(float);
    
    for (int i = 0; i < samples; i++) {
        float sample = 0.0f;
        for (int ch = 0; ch < 4; ch++) {
            if (apu.duration[ch] > 0 && apu.vol[ch] > 0) {
                float t = apu.phase[ch];
                float s = 0.0f;
                // Wave generation
                switch (apu.wave[ch]) {
                    case 0: s = (t < 0.5f) ? 1.0f : -1.0f; break; // Square
                    case 1: s = sinf(t * 6.28318f); break;        // Sine
                    case 2: s = (t < 0.5f) ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t); break; // Triangle
                    case 3: s = ((float)rand() / RAND_MAX) * 2.0f - 1.0f; break; // Noise
                }
                sample += s * apu.vol[ch];
                
                // Phase increment
                apu.phase[ch] += apu.freq[ch] / 48000.0f;
                if (apu.phase[ch] >= 1.0f) apu.phase[ch] -= 1.0f;
            }
        }
        buffer[i] = sample * 0.25f; // Mix
    }
}

namespace zenu {
    void audio_play_note(int channel, float freq, float volume, WaveType wave_type) {
        if (channel < 0 || channel >= 4) return;
        SDL_LockAudioDevice(pc.audioDevice);
        apu.freq[channel] = freq;
        apu.vol[channel] = volume;
        apu.wave[channel] = (u8)wave_type;
        apu.duration[channel] = 60; // Default duration
        apu.phase[channel] = 0.0f;
        SDL_UnlockAudioDevice(pc.audioDevice);
    }
    
    void audio_stop(int channel) {
        if (channel < 0 || channel >= 4) return;
        SDL_LockAudioDevice(pc.audioDevice);
        apu.vol[channel] = 0;
        apu.duration[channel] = 0;
        SDL_UnlockAudioDevice(pc.audioDevice);
    }

    void gfx_init() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) exit(1);
        pc.window = SDL_CreateWindow("Zenu Engine (PC)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 576, SDL_WINDOW_SHOWN);
        pc.renderer = SDL_CreateRenderer(pc.window, -1, SDL_RENDERER_ACCELERATED);
        SDL_RenderSetLogicalSize(pc.renderer, 160, 144);
        pc.running = true;

        // Audio Init
        SDL_AudioSpec want, have;
        SDL_zero(want);
        want.freq = 48000;
        want.format = AUDIO_F32;
        want.channels = 1;
        want.samples = 1024;
        want.callback = audio_callback;
        pc.audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        SDL_PauseAudioDevice(pc.audioDevice, 0);

        // Controller Init
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (SDL_IsGameController(i)) {
                pc.controller = SDL_GameControllerOpen(i);
                if (pc.controller) break;
            }
        }
    }

    void gfx_begin_frame() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) pc.running = false;
        }
        // Decrement audio timers
        SDL_LockAudioDevice(pc.audioDevice);
        for(int i=0; i<4; i++) if(apu.duration[i] > 0) apu.duration[i]--;
        SDL_UnlockAudioDevice(pc.audioDevice);
    }

    void gfx_clear(Color color) {
        SDL_SetRenderDrawColor(pc.renderer, color.r, color.g, color.b, color.a);
        SDL_RenderClear(pc.renderer);
    }

    void gfx_draw_rect(i32 x, i32 y, i32 w, i32 h, Color color) {
        SDL_SetRenderDrawColor(pc.renderer, color.r, color.g, color.b, color.a);
        SDL_Rect rect = {x, y, w, h};
        SDL_RenderFillRect(pc.renderer, &rect);
    }
    
    void gfx_draw_line(i32 x1, i32 y1, i32 x2, i32 y2, Color color) {
        SDL_SetRenderDrawColor(pc.renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(pc.renderer, x1, y1, x2, y2);
    }

    void gfx_draw_triangle(i32 x1, i32 y1, i32 x2, i32 y2, i32 x3, i32 y3, Color color) {
        // Approximate triangle for PC (just draw edges for now to be simple, or lines)
        gfx_draw_line(x1, y1, x2, y2, color);
        gfx_draw_line(x2, y2, x3, y3, color);
        gfx_draw_line(x3, y3, x1, y1, color);
    }
    
    void gfx_draw_char(i32 x, i32 y, char c, Color color) {
        if (c < 32 || c > 127) return;
        const u8* glyph = font_8x8[c - 32];
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (glyph[row] & (0x80 >> col)) {
                    gfx_draw_rect(x + col, y + row, 1, 1, color);
                }
            }
        }
    }

    void gfx_draw_text(i32 x, i32 y, const char* text, Color color) {
         int cx = x;
         while (*text) {
             if (*text == '\n') { y += 9; cx = x; }
             else { gfx_draw_char(cx, y, *text, color); cx += 8; }
             text++;
         }
    }

    void gfx_end_frame() {
        SDL_RenderPresent(pc.renderer);
        SDL_Delay(16); // ~60 FPS cap
    }

    bool input_is_down(Button btn) {
        const Uint8* k = SDL_GetKeyboardState(NULL);
        bool pressed = false;
        
        switch (btn) {
            case BUTTON_UP:    pressed = k[SDL_SCANCODE_UP]    || (pc.controller && SDL_GameControllerGetButton(pc.controller, SDL_CONTROLLER_BUTTON_DPAD_UP)); break;
            case BUTTON_DOWN:  pressed = k[SDL_SCANCODE_DOWN]  || (pc.controller && SDL_GameControllerGetButton(pc.controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN)); break;
            case BUTTON_LEFT:  pressed = k[SDL_SCANCODE_LEFT]  || (pc.controller && SDL_GameControllerGetButton(pc.controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT)); break;
            case BUTTON_RIGHT: pressed = k[SDL_SCANCODE_RIGHT] || (pc.controller && SDL_GameControllerGetButton(pc.controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)); break;
            case BUTTON_A:     pressed = k[SDL_SCANCODE_Z]     || (pc.controller && SDL_GameControllerGetButton(pc.controller, SDL_CONTROLLER_BUTTON_A)); break;
            case BUTTON_B:     pressed = k[SDL_SCANCODE_X]     || (pc.controller && SDL_GameControllerGetButton(pc.controller, SDL_CONTROLLER_BUTTON_B)); break;
            case BUTTON_START: pressed = k[SDL_SCANCODE_RETURN]|| (pc.controller && SDL_GameControllerGetButton(pc.controller, SDL_CONTROLLER_BUTTON_START)); break;
            case BUTTON_SELECT:pressed = k[SDL_SCANCODE_RSHIFT]|| (pc.controller && SDL_GameControllerGetButton(pc.controller, SDL_CONTROLLER_BUTTON_BACK)); break;
        }
        return pressed;
    }
    
    // Simple state tracking for just_pressed
    static u8 prev_input = 0;
    static u8 curr_input = 0;
    
    void update_input_state() {
        prev_input = curr_input;
        curr_input = 0;
        if (input_is_down(BUTTON_UP))     curr_input |= BUTTON_UP;
        if (input_is_down(BUTTON_DOWN))   curr_input |= BUTTON_DOWN;
        if (input_is_down(BUTTON_LEFT))   curr_input |= BUTTON_LEFT;
        if (input_is_down(BUTTON_RIGHT))  curr_input |= BUTTON_RIGHT;
        if (input_is_down(BUTTON_A))      curr_input |= BUTTON_A;
        if (input_is_down(BUTTON_B))      curr_input |= BUTTON_B;
        if (input_is_down(BUTTON_START))  curr_input |= BUTTON_START;
        if (input_is_down(BUTTON_SELECT)) curr_input |= BUTTON_SELECT;
    }

    bool input_just_pressed(Button btn) {
        return (curr_input & btn) && !(prev_input & btn);
    }
}

int main() {
    zenu::gfx_init();
    ::game_init();
    while (pc.running) {
        zenu::gfx_begin_frame();
        zenu::update_input_state();
        ::game_update();
        ::game_draw();
        zenu::gfx_end_frame();
    }
    return 0;
}
#endif
