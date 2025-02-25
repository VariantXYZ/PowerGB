#include <cstddef>

#include <cpu/include/register.hpp>
#include <test/include/acutest.h>

using namespace pgb::cpu;

template <typename Datatype>
void test_register_type(void);
void test_register8_access4(void);
void test_register16_access4(void);
void test_register16_access8(void);

TEST_LIST = {
    {"RegisterWidth 8 AccessWidth 4", test_register8_access4},
    {"RegisterWidth 8 AccessWidth 8", test_register_type<datatypes::Byte>},
    //{"RegisterWidth 16 AccessWidth 4", test_register16_access4},
    //{"RegisterWidth 16 AccessWidth 8", test_register16_access8},
    {"RegisterWidth 16 AccessWidth 16", test_register_type<datatypes::Word>},
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
    reg.template Self<0>() = Datatype::MaxValue;
    TEST_CHECK(reg.template Self<0>() == Datatype::MaxValue);
}

void test_register8_access4(void)
{
    Register<8, 4> reg;

    // Initialization
    TEST_CHECK(reg.Nibble<0>() == 0);
    TEST_CHECK(reg.Nibble<1>() == 0);
    TEST_CHECK(reg.Byte<0>() == 0);

    // Reference Nibbles directly
    auto& high = reg.Nibble<0>();
    auto& low  = reg.Nibble<1>();
    high       = 0x1;
    TEST_CHECK(reg.Byte<0>() == 0x10);
    low = 0x2;
    TEST_CHECK(reg.Byte<0>() == 0x12);

    // Set value via Byte
    reg.Byte<0>(0x00);
    TEST_CHECK(reg.Byte<0>() == 0x00);

    // Set value via Nibble
    reg.Byte<0>(0xD, 0xA);
    TEST_CHECK(reg.Byte<0>() == 0xDA);
}
