#include <cstddef>

#include <cpu/include/register.hpp>
#include <test/include/acutest.h>

using namespace pgb::cpu;

template <typename Datatype>
void test_register_type(void);

template <std::size_t AccessWidth>
void test_register8(void);
void test_register8_access4(void);
void test_register8_access8(void);

template <std::size_t AccessWidth>
void test_register16(void);
void test_register16_access4(void);
void test_register16_access8(void);
void test_register16_access16(void);
TEST_LIST = {
    {"RegisterWidth 8 AccessWidth 4", test_register8_access4},
    {"RegisterWidth 8 AccessWidth 8", test_register8_access8},
    {"RegisterWidth 16 AccessWidth 4", test_register16_access4},
    {"RegisterWidth 16 AccessWidth 8", test_register16_access8},
    {"RegisterWidth 16 AccessWidth 16", test_register16_access16},
    {NULL, NULL}
};

// If the size and access width are the same, the tests are simple
// Datatype tests themselves are handled elsewhere
template <typename Datatype>
void test_register_type(void)
{
    Register<Datatype::TypeWidth, Datatype::TypeWidth> reg;

    // Initialization, set, and check
    TEST_CHECK(reg.template Self<0>() == Datatype::MinValue);
    auto& self = reg.template Self<0>();
    self       = Datatype::MaxValue;
    TEST_CHECK(reg.template Self<0>() == Datatype::MaxValue);
}

template <std::size_t AccessWidth>
void test_register8(Register<8, AccessWidth>& reg)
{
    // Initialization
    TEST_CHECK(reg.template Nibble<0>() == 0);
    TEST_CHECK(reg.template Nibble<1>() == 0);
    TEST_CHECK(reg.template Byte<0>() == 0);

    // Set Nibbles directly
    reg.template Nibble<0>(0x1);
    TEST_CHECK(reg.template Byte<0>() == 0x10);
    reg.template Nibble<1>(0x2);
    TEST_CHECK(reg.template Byte<0>() == 0x12);

    // Set value via Byte
    reg.template Byte<0>(0x34);
    TEST_CHECK(reg.template Byte<0>() == 0x34);

    // Set back to 0 before returning
    reg.template Byte<0>(0);
    TEST_CHECK(reg.template Byte<0>() == 0);
}

void test_register8_access4(void)
{
    Register<8, 4> reg;

    test_register8(reg);

    // Reference Nibbles directly
    auto& high = reg.Nibble<0>();
    auto& low  = reg.Nibble<1>();
    high       = 0x1;
    TEST_CHECK(reg.Byte<0>() == 0x10);
    low = 0x2;
    TEST_CHECK(reg.Byte<0>() == 0x12);

    // Set value via Nibble
    reg.Byte<0>(0xD, 0xA);
    TEST_CHECK(reg.Byte<0>() == 0xDA);
}

void test_register8_access8(void)
{
    test_register_type<datatypes::Byte>();

    Register<8, 8> reg;

    test_register8(reg);
}

template <std::size_t AccessWidth>
void test_register16(Register<16, AccessWidth>& reg)
{
    // Initialization
    TEST_CHECK(reg.template Nibble<0>() == 0);
    TEST_CHECK(reg.template Nibble<1>() == 0);
    TEST_CHECK(reg.template Nibble<2>() == 0);
    TEST_CHECK(reg.template Nibble<3>() == 0);
    TEST_CHECK(reg.template Byte<0>() == 0);
    TEST_CHECK(reg.template Byte<1>() == 0);
    TEST_CHECK(reg.template Word<0>() == 0);

    // Set Nibbles directly
    reg.template Nibble<0>(0x1);
    TEST_CHECK(reg.template Word<0>() == 0x1000);
    reg.template Nibble<1>(0x2);
    TEST_CHECK(reg.template Word<0>() == 0x1200);
    reg.template Nibble<2>(0x3);
    TEST_CHECK(reg.template Word<0>() == 0x1230);
    reg.template Nibble<3>(0x4);
    TEST_CHECK(reg.template Word<0>() == 0x1234);

    // Set Word via Bytes
    reg.template Word<0>(0x34, 0x12);
    TEST_CHECK(reg.template Word<0>() == 0x3412);

    // Set Word directly
    reg.template Word<0>(0x4567);
    TEST_CHECK(reg.template Nibble<0>() == 4);
    TEST_CHECK(reg.template Nibble<1>() == 5);
    TEST_CHECK(reg.template Nibble<2>() == 6);
    TEST_CHECK(reg.template Nibble<3>() == 7);
    TEST_CHECK(reg.template Byte<0>() == 0x45);
    TEST_CHECK(reg.template Byte<1>() == 0x67);
    TEST_CHECK(reg.template Word<0>() == 0x4567);

    // Set back to 0 before returning
    reg.template Word<0>(0);
    TEST_CHECK(reg.template Word<0>() == 0);
}

void test_register16_access4(void)
{
    Register<16, 4> reg;

    test_register16(reg);

    // Reference Nibbles directly
    auto& n0 = reg.Nibble<0>();
    auto& n1 = reg.Nibble<1>();
    auto& n2 = reg.Nibble<2>();
    auto& n3 = reg.Nibble<3>();

    n0       = 0x1;
    TEST_CHECK(reg.Word<0>() == 0x1000);
    n1 = 0x2;
    TEST_CHECK(reg.Word<0>() == 0x1200);
    n2 = 0x3;
    TEST_CHECK(reg.Word<0>() == 0x1230);
    n3 = 0x4;
    TEST_CHECK(reg.Word<0>() == 0x1234);
}

void test_register16_access8(void)
{
    Register<16, 8> reg;

    test_register16(reg);

    // Reference Bytes directly
    auto& b0 = reg.Byte<0>();
    auto& b1 = reg.Byte<1>();

    b0       = 0x12;
    TEST_CHECK(reg.Word<0>() == 0x1200);
    b1 = 0x34;
    TEST_CHECK(reg.Word<0>() == 0x1234);
}

void test_register16_access16(void)
{
    test_register_type<datatypes::Word>();

    Register<16, 16> reg;
    test_register16(reg);
}
