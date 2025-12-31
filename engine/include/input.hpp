#ifndef INPUT_HPP
#define INPUT_HPP

#include "types.hpp"

namespace zenu {
    enum Button {
        BUTTON_UP     = (1 << 0),
        BUTTON_DOWN   = (1 << 1),
        BUTTON_LEFT   = (1 << 2),
        BUTTON_RIGHT  = (1 << 3),
        BUTTON_A      = (1 << 4),
        BUTTON_B      = (1 << 5),
        BUTTON_START  = (1 << 6),
        BUTTON_SELECT = (1 << 7)
    };

    enum AnalogAxis {
        AXIS_LEFT_X,
        AXIS_LEFT_Y,
        AXIS_RIGHT_X,
        AXIS_RIGHT_Y
    };

    bool input_is_down(Button btn);
    bool input_just_pressed(Button btn);
    float input_get_axis(AnalogAxis axis);
    
    // Internal use by backend
    void update_input_state();
}

#endif
