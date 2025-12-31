#ifndef ZENU_ENGINE_HPP
#define ZENU_ENGINE_HPP

#include "types.hpp"
#include "math.hpp"
#include "gfx.hpp"
#include "input.hpp"
#include "audio.hpp"

// User-implemented hooks
extern void game_init();
extern void game_update();
extern void game_draw();

namespace zenu {
    // Engine internals
    void run(); 
    void quit();
}

#endif
