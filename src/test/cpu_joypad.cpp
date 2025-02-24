#include "../cpu/include/joypad.hpp"

#include "include/acutest.h"

void test_JOYPToState(void);

TEST_LIST = {
    {"StateToJOYP", test_JOYPToState},
    {NULL, NULL}
};

void test_JOYPToState(void)
{
    using namespace pgb::cpu;
    Joypad::JoypadState jp = Joypad::JOYPToState(0xD7);
    
    TEST_CHECK(jp == Joypad::StartPressed);
}