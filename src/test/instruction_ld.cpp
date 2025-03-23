#include <cpu/include/instruction_ld.hpp>
#include <memory/include/memory.hpp>
#include <test/include/acutest.h>

void test_basic_ld(void);

TEST_LIST = {
    {"Basic ld", test_basic_ld},
    {NULL, NULL}
};

using namespace pgb::cpu;
using namespace pgb::memory;

static auto registers = RegisterFile();
static auto mmap      = MemoryMap(registers, MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount);

void test_basic_ld(void)
{
    mmap.Reset();
    TEST_ASSERT(mmap.Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

    auto r1 = mmap.ReadByte(0x6000);
    TEST_ASSERT(r1.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(r1) == 0x00);

    TEST_ASSERT(mmap.WriteByte(RegisterType::A, 0x50).IsSuccess());
    TEST_ASSERT(mmap.WriteWord(RegisterType::BC, 0x6000).IsSuccess());
    TEST_ASSERT(instruction::Ld_BC_A_Decoder::Execute(mmap) == 4);

    auto r2 = mmap.ReadByte(0x6000);
    TEST_ASSERT(r2.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(r2) == 0x50);

    TEST_ASSERT(mmap.WriteByte(RegisterType::A, 0x01).IsSuccess());
    auto r3 = mmap.ReadByte(RegisterType::A);
    TEST_ASSERT(r3.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(r3) == 0x01);

    TEST_ASSERT(instruction::Ld_A_BC_Decoder::Execute(mmap) == 4);
    auto r4 = mmap.ReadByte(RegisterType::A);
    TEST_ASSERT(r4.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(r4) == 0x50);
}