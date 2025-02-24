#include "include/joypad.hpp"

namespace pgb::cpu
{

namespace
{
// JP state is either requested & read in groups of Start/Select/A/B (inclusive or) Down/Up/Left/Right
// That is to say, with one call:
// 	* it is not possible to check if both Start and Down are pressed
//  * it is possible to check that Start and Select are pressed)
//  * It is possible to check that either Start or Down is pressed
enum JoypadStateInternal : uint8_t
{
    // Another caveat is that JP state is default pulled high, so 1 is actually 'off/not pressed'
    // [ignored:2][Read Buttons group:1][Read D-pad group: 1][Start/Down pressed:1][Select/Up pressed:1][B/Left pressed: 1][A/Right pressed: 1]
    All    = static_cast<uint8_t>(~0b11000000),

    // Buttons group
    Start  = static_cast<uint8_t>(~0b11010111),
    Select = static_cast<uint8_t>(~0b11011011),
    B      = static_cast<uint8_t>(~0b11011101),
    A      = static_cast<uint8_t>(~0b11011110),

    // D-Pad group
    Down   = static_cast<uint8_t>(~0b11100111),
    Up     = static_cast<uint8_t>(~0b11101011),
    Left   = static_cast<uint8_t>(~0b11101101),
    Right  = static_cast<uint8_t>(~0b11101110),

    None   = static_cast<uint8_t>(~0b11111111),
};

} // namespace

// static
Joypad::JoypadState Joypad::JOYPToState(uint8_t state)
{
    return static_cast<JoypadState>(
        ~(
            // Buttons
            (state & JoypadStateInternal::Start ? JoypadState::StartPressed : 0) |
            (state & JoypadStateInternal::Select ? JoypadState::SelectPressed : 0) |
            (state & JoypadStateInternal::B ? JoypadState::BPressed : 0) |
            (state & JoypadStateInternal::A ? JoypadState::APressed : 0) |

            // D-Pad
            (state & JoypadStateInternal::Down ? JoypadState::DownPressed : 0) |
            (state & JoypadStateInternal::Up ? JoypadState::UpPressed : 0) |
            (state & JoypadStateInternal::Left ? JoypadState::LeftPressed : 0) |
            (state & JoypadStateInternal::Right ? JoypadState::RightPressed : 0)
        )
    );
}

} // namespace pgb::cpu