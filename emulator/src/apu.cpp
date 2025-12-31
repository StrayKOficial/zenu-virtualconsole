#include "apu.hpp"
#include <iostream>

APU::APU() : deviceId(0) {}

APU::~APU() {
    cleanup();
}

bool APU::init() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Audio could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_memset(&want, 0, sizeof(want));
    want.freq = 48000; // Hi-Fi Standard
    want.format = AUDIO_F32SYS; // 32-bit Floating Point Audio
    want.channels = 1; // Mono for now
    want.samples = 1024;
    want.callback = audio_callback;
    want.userdata = this;

    deviceId = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (deviceId == 0) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_PauseAudioDevice(deviceId, 0);
    return true;
}

void APU::cleanup() {
    if (deviceId != 0) {
        SDL_CloseAudioDevice(deviceId);
        deviceId = 0;
    }
}

void APU::write8(uint32_t addr, uint8_t data) {
    uint32_t reg = addr & 0xFF;
    
    SDL_LockAudioDevice(deviceId);
    if (reg < 0x20) { // Legacy Wave Channels
        int ch = reg / 4;
        int sub = reg % 4;
        if (sub == 0) freq_raw[ch] = (freq_raw[ch] & 0xFF00) | data;
        else if (sub == 1) freq_raw[ch] = (freq_raw[ch] & 0x00FF) | (data << 8);
        else if (sub == 2) {
            channels[ch].enabled = (data & 1);
            channels[ch].type = (data >> 1) & 0x3; // 0=Square, 1=Sine, 2=Triangle
            if (channels[ch].enabled) channels[ch].phase = 0;
        } else if (sub == 3) {
            channels[ch].volume = (float)data / 255.0f;
        }

        if (freq_raw[ch] > 0) channels[ch].frequency = (float)freq_raw[ch];
        else channels[ch].frequency = 0;
    }
    SDL_UnlockAudioDevice(deviceId);
}

uint8_t APU::read8(uint32_t addr) {
    return 0;
}

void APU::audio_callback(void* userdata, uint8_t* stream, int len) {
    APU* apu = (APU*)userdata;
    float* buffer = (float*)stream; // 32-bit float buffer
    int samples = len / sizeof(float);

    for (int i = 0; i < samples; i++) {
        float out = 0.0f;
        for (int c = 0; c < 4; c++) {
            if (!apu->channels[c].enabled || apu->channels[c].frequency <= 0) continue;

            float sample = 0.0f;
            if (apu->channels[c].type == 0) { // Square
                sample = (apu->channels[c].phase < 0.5f) ? 1.0f : -1.0f;
            } else if (apu->channels[c].type == 1) { // Sine (Smooth Hi-Fi)
                sample = std::sin(apu->channels[c].phase * 2.0f * M_PI);
            } else if (apu->channels[c].type == 2) { // Triangle
                sample = 4.0f * std::abs(apu->channels[c].phase - 0.5f) - 1.0f;
            }

            out += sample * apu->channels[c].volume * 0.25f; // Mix 4 channels

            apu->channels[c].phase += apu->channels[c].frequency / 48000.0f;
            while (apu->channels[c].phase >= 1.0f) apu->channels[c].phase -= 1.0f;
        }
        
        buffer[i] = out; // Direct 32-bit float output
    }
}
