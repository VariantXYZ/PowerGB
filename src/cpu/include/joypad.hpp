#pragma once

#include <cstdint>
namespace pgb::cpu
{

class Joypad
{
public:
    // Provide which buttons are pressed
    enum JoypadState : uint8_t
    {
        NonePressed   = 0x0,

        StartPressed  = 1 << 0,
        SelectPressed = 1 << 1,
        APressed      = 1 << 2,
        BPressed      = 1 << 3,
        DownPressed   = 1 << 4,
        UpPressed     = 1 << 5,
        LeftPressed   = 1 << 6,
        RightPressed  = 1 << 7,

        AllPressed    = 0xFF,
    };

    // Take the value of the JOYP register and return what values are pressed and requested
    static JoypadState JOYPToState(uint8_t);
};

} // namespace pgb::cpu