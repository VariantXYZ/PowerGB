#include <cstddef>

#include <common/datatypes.hpp>
#include <test/include/acutest.h>

using namespace pgb::common::datatypes;

template <typename D>
void test_basic_datatype(void);
void test_byte(void);
void test_word(void);

TEST_LIST = {
    {"Basic Nibble", test_basic_datatype<Nibble>},
    {"Basic Byte", test_basic_datatype<Byte>},
    {"Basic Word", test_basic_datatype<Word>},
    {"Test Byte Functions", test_byte},
    {"Test Word Functions", test_word},

    {NULL, NULL}
};

template <typename D>
void test_basic_datatype(void)
{
    D d1(1);
    D d2;

    // Initialization/constructor and casting
    TEST_CHECK(d1.data == 1);
    TEST_CHECK(d2.data == D::MinValue);
    TEST_CHECK(static_cast<D::Type>(d1) == 1);
    TEST_CHECK(d2 == D::MinValue);

    // Setting value
    d1.data = D::MaxValue;
    TEST_CHECK(d1.data == D::MaxValue);

    // Operators and defined "overflow" behavior
    TEST_CHECK(d1++ == D::MaxValue);
    TEST_CHECK(d1 == D::MinValue);
    TEST_CHECK(d2-- == D::MinValue);
    TEST_CHECK(d2 == D::MaxValue);
    TEST_CHECK(--d1 == d2);
    TEST_CHECK(++d2 == D::MinValue);

    // Copy constructor
    D d3(d2);
    TEST_CHECK(--d2 == D::MaxValue && d3 == D::MinValue);
}

void test_byte(void)
{
    Byte b0(0x1, 0xF);

    TEST_CHECK(b0 == 0x1F);
    TEST_CHECK(b0.HighNibble() == 0x1);
    TEST_CHECK(b0.LowNibble() == 0xF);
    b0.HighNibble(0x2);
    TEST_CHECK(b0 == 0x2F);
    b0.LowNibble(0x3);
    TEST_CHECK(b0 == 0x23);
}

void test_word(void)
{
    Word w(0x11, 0xFF);

    TEST_CHECK(w == 0x11FF);
    TEST_CHECK(w.HighByte() == 0x11);
    TEST_CHECK(w.LowByte() == 0xFF);
    w.HighByte(0x20);
    TEST_CHECK(w == 0x20FF);
    w.LowByte(0xED);
    TEST_CHECK(w == 0x20ED);
}
