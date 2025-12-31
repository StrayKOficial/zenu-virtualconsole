#ifndef ZENU_AUDIO_HPP
#define ZENU_AUDIO_HPP

#include "types.hpp"

namespace zenu {
    enum WaveType {
        WAVE_SQUARE   = 0,
        WAVE_SINE     = 1,
        WAVE_TRIANGLE = 2
    };

    void audio_play_note(int channel, float frequency, float volume, WaveType type);
    void audio_stop(int channel);
}

#endif
