#ifndef APU_HPP
#define APU_HPP

#include <SDL2/SDL.h>
#include <cstdint>
#include <vector>
#include <cmath>

class APU {
public:
    APU();
    ~APU();

    bool init();
    void cleanup();

    // Memory-mapped register interface
    void write8(uint32_t addr, uint8_t data);
    uint8_t read8(uint32_t addr);

    // Audio callback for SDL
    static void audio_callback(void* userdata, uint8_t* stream, int len);

private:
    SDL_AudioSpec want, have;
    SDL_AudioDeviceID deviceId;

    // Pulse/Sine Wave Channel (Hi-Fi)
    struct Channel {
        bool enabled = false;
        float frequency = 0.0f;
        float volume = 0.5f;
        float phase = 0.0f;
        int type = 0; // 0=Square, 1=Sine, 2=Triangle
    } channels[4];

    uint32_t freq_raw[4] = {0, 0, 0, 0};
};

#endif
