#include "../cpu/include/cpu.hpp"

#include <type_traits>

#include "include/acutest.h"

void test_cpu_register_gp(void);
void test_cpu_register_sm(void);
void test_cpu_register_af(void);

TEST_LIST = {
    {"General Purpose Registers", test_cpu_register_gp},
    {"State Management Registers", test_cpu_register_sm},
    {"Accumulator/Flag Registers", test_cpu_register_af},

    {NULL, NULL}};

void test_cpu_register_gp(void)
{
    pgb::cpu::CPU cpu;

    // 16-bit init
    TEST_CHECK(cpu.BC == 0);
    TEST_CHECK(cpu.DE == 0);
    TEST_CHECK(cpu.HL == 0);

    // 8-bit init
    TEST_CHECK(cpu.B == 0);
    TEST_CHECK(cpu.C == 0);
    TEST_CHECK(cpu.D == 0);
    TEST_CHECK(cpu.E == 0);
    TEST_CHECK(cpu.H == 0);
    TEST_CHECK(cpu.L == 0);

    // 16-bit set
    cpu.BC = 0xFEEF;
    TEST_CHECK(cpu.BC == 0xFEEF);
    TEST_CHECK(cpu.B == 0xFE);
    TEST_CHECK(cpu.C == 0xEF);

    cpu.DE = 0xFEEF;
    TEST_CHECK(cpu.DE == 0xFEEF);
    TEST_CHECK(cpu.D == 0xFE);
    TEST_CHECK(cpu.E == 0xEF);

    cpu.HL = 0xFEEF;
    TEST_CHECK(cpu.HL == 0xFEEF);
    TEST_CHECK(cpu.H == 0xFE);
    TEST_CHECK(cpu.L == 0xEF);

    // 8-bit set
    cpu.B = 0xEF;
    cpu.C = 0XFE;
    TEST_CHECK(cpu.BC == 0xEFFE);
    TEST_CHECK(cpu.B == 0xEF);
    TEST_CHECK(cpu.C == 0XFE);

    cpu.D = 0xEF;
    cpu.E = 0XFE;
    TEST_CHECK(cpu.DE == 0xEFFE);
    TEST_CHECK(cpu.D == 0xEF);
    TEST_CHECK(cpu.E == 0XFE);

    cpu.H = 0xEF;
    cpu.L = 0XFE;
    TEST_CHECK(cpu.HL == 0xEFFE);
    TEST_CHECK(cpu.H == 0xEF);
    TEST_CHECK(cpu.L == 0XFE);
}

void test_cpu_register_sm(void)
{
    pgb::cpu::CPU cpu;

    // 16-bit init
    TEST_CHECK(cpu.SP == 0);
    TEST_CHECK(cpu.PC == 0);

    // 16-bit set
    cpu.SP = 0xFEEF;
    TEST_CHECK(cpu.SP == 0xFEEF);

    cpu.PC = 0xFEEF;
    TEST_CHECK(cpu.PC == 0xFEEF);
}

void test_cpu_register_af(void)
{
    pgb::cpu::CPU cpu;

    // 8-bit init
    TEST_CHECK(cpu.A == 0);
    TEST_CHECK(!cpu.FlagZ());
    TEST_CHECK(!cpu.FlagN());
    TEST_CHECK(!cpu.FlagH());
    TEST_CHECK(!cpu.FlagC());

    // 8-bit set
    cpu.A = 0xFE;
    TEST_CHECK(cpu.A == 0xFE);

    // Flag set
    cpu.FlagSetZ();
    TEST_CHECK(cpu.FlagZ());
    cpu.FlagSetN();
    TEST_CHECK(cpu.FlagN());
    cpu.FlagSetH();
    TEST_CHECK(cpu.FlagH());
    cpu.FlagSetC();
    TEST_CHECK(cpu.FlagC());

    // Flag reset
    cpu.FlagReset();
    TEST_CHECK(!cpu.FlagZ());
    TEST_CHECK(!cpu.FlagN());
    TEST_CHECK(!cpu.FlagH());
    TEST_CHECK(!cpu.FlagC());
}