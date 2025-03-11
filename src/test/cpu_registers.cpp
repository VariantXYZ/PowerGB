#include <cpu/include/registers.hpp>
#include <test/include/acutest.h>

TEST_LIST = {
    {NULL, NULL}
};

// Actual functionality is determined by the memory and datatype tests
// This entire 'test' is actually just to make sure definitions are correct

static constexpr const pgb::cpu::RegisterFile registers;

// Check expected type widths
static_assert(registers.IR().TypeWidth == 8);
static_assert(registers.IE().TypeWidth == 8);
static_assert(registers.A().TypeWidth == 8);
static_assert(registers.F().TypeWidth == 4);
static_assert(registers.B().TypeWidth == 8);
static_assert(registers.C().TypeWidth == 8);
static_assert(registers.D().TypeWidth == 8);
static_assert(registers.E().TypeWidth == 8);
static_assert(registers.H().TypeWidth == 8);
static_assert(registers.L().TypeWidth == 8);

static_assert(registers.AF().TypeWidth == 16);
static_assert(registers.BC().TypeWidth == 16);
static_assert(registers.DE().TypeWidth == 16);
static_assert(registers.HL().TypeWidth == 16);
static_assert(registers.PC().TypeWidth == 16);
static_assert(registers.SP().TypeWidth == 16);