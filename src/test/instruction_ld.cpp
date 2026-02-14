#include <cpu/include/instruction_ld.hpp>
#include <memory/include/memory.hpp>
#include <test/include/acutest.h>

void test_identity(void);
void test_bc_a(void);
void test_b_a(void);

TEST_LIST = {
    {"Identity", test_identity},
    {"ld [bc], a & ld a, [bc]", test_bc_a},
    {"ld a, b & ld b, a", test_b_a},
    {NULL, NULL}
};

using namespace pgb::common;
using namespace pgb::cpu;
using namespace pgb::memory;

// NOP + ld instructions
// Note how we explicitly only include instruction_ld in this test
static_assert(instruction::InstructionRegistryNoPrefix::Size == 89);

static auto registers = RegisterFile();
static auto mmap      = MemoryMap(registers, MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount);

void test_identity(void)
{
    {
        mmap.Reset();
        TEST_ASSERT(mmap.Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

        const std::size_t val = 0x12;
        TEST_ASSERT(mmap.WriteByte(RegisterType::A, val).IsSuccess());
        TEST_ASSERT(instruction::Ld_A_A_Decoder::Execute(mmap) == 4);
        auto r1 = mmap.ReadByte(RegisterType::A);
        TEST_ASSERT(static_cast<const Byte&>(r1) == val);
    }

    {
        mmap.Reset();
        TEST_ASSERT(mmap.Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

        const std::size_t val = 0x34;
        TEST_ASSERT(mmap.WriteByte(RegisterType::B, val).IsSuccess());
        TEST_ASSERT(instruction::Ld_B_B_Decoder::Execute(mmap) == 4);
        auto r1 = mmap.ReadByte(RegisterType::B);
        TEST_ASSERT(static_cast<const Byte&>(r1) == val);
    }

    {
        mmap.Reset();
        TEST_ASSERT(mmap.Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

        const std::size_t val = 0x56;
        TEST_ASSERT(mmap.WriteByte(RegisterType::C, val).IsSuccess());
        TEST_ASSERT(instruction::Ld_C_C_Decoder::Execute(mmap) == 4);
        auto r1 = mmap.ReadByte(RegisterType::C);
        TEST_ASSERT(static_cast<const Byte&>(r1) == val);
    }

    {
        mmap.Reset();
        TEST_ASSERT(mmap.Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

        const std::size_t val = 0x78;
        TEST_ASSERT(mmap.WriteByte(RegisterType::D, val).IsSuccess());
        TEST_ASSERT(instruction::Ld_D_D_Decoder::Execute(mmap) == 4);
        auto r1 = mmap.ReadByte(RegisterType::D);
        TEST_ASSERT(static_cast<const Byte&>(r1) == val);
    }

    {
        mmap.Reset();
        TEST_ASSERT(mmap.Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

        const std::size_t val = 0x9A;
        TEST_ASSERT(mmap.WriteByte(RegisterType::E, val).IsSuccess());
        TEST_ASSERT(instruction::Ld_E_E_Decoder::Execute(mmap) == 4);
        auto r1 = mmap.ReadByte(RegisterType::E);
        TEST_ASSERT(static_cast<const Byte&>(r1) == val);
    }

    {
        mmap.Reset();
        TEST_ASSERT(mmap.Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

        const std::size_t val = 0xBC;
        TEST_ASSERT(mmap.WriteByte(RegisterType::H, val).IsSuccess());
        TEST_ASSERT(instruction::Ld_H_H_Decoder::Execute(mmap) == 4);
        auto r1 = mmap.ReadByte(RegisterType::H);
        TEST_ASSERT(static_cast<const Byte&>(r1) == val);
    }

    {
        mmap.Reset();
        TEST_ASSERT(mmap.Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

        const std::size_t val = 0xDE;
        TEST_ASSERT(mmap.WriteByte(RegisterType::L, val).IsSuccess());
        TEST_ASSERT(instruction::Ld_L_L_Decoder::Execute(mmap) == 4);
        auto r1 = mmap.ReadByte(RegisterType::L);
        TEST_ASSERT(static_cast<const Byte&>(r1) == val);
    }
}

void test_bc_a(void)
{
    mmap.Reset();
    TEST_ASSERT(mmap.Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

    auto r1 = mmap.ReadByte(0x6000);
    TEST_ASSERT(r1.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(r1) == 0x00);

    TEST_ASSERT(mmap.WriteByte(RegisterType::A, 0x50).IsSuccess());
    TEST_ASSERT(mmap.WriteWord(RegisterType::BC, 0x6000).IsSuccess());
    TEST_ASSERT(instruction::Ld_BC_A_Decoder::Execute(mmap) == 8);

    auto r2 = mmap.ReadByte(0x6000);
    TEST_ASSERT(r2.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(r2) == 0x50);

    TEST_ASSERT(mmap.WriteByte(RegisterType::A, 0x01).IsSuccess());
    auto r3 = mmap.ReadByte(RegisterType::A);
    TEST_ASSERT(r3.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(r3) == 0x01);

    TEST_ASSERT(instruction::Ld_A_BC_Decoder::Execute(mmap) == 8);
    auto r4 = mmap.ReadByte(RegisterType::A);
    TEST_ASSERT(r4.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(r4) == 0x50);
}

void test_b_a(void)
{
    mmap.Reset();
    TEST_ASSERT(mmap.Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

    TEST_ASSERT(mmap.WriteByte(RegisterType::A, 0xF3).IsSuccess());
    auto r1 = mmap.ReadByte(RegisterType::A);
    TEST_ASSERT(r1.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(r1) == 0xF3);

    auto r2 = mmap.ReadByte(RegisterType::B);
    TEST_ASSERT(r2.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(r2) == 0x00);

    TEST_ASSERT(instruction::Ld_B_A_Decoder::Execute(mmap) == 4);
    auto r3 = mmap.ReadByte(RegisterType::B);
    TEST_ASSERT(r3.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(r3) == 0xF3);

    TEST_ASSERT(mmap.WriteByte(RegisterType::A, 0x00).IsSuccess());
    auto r4 = mmap.ReadByte(RegisterType::A);
    TEST_ASSERT(r4.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(r4) == 0x00);

    TEST_ASSERT(instruction::Ld_A_B_Decoder::Execute(mmap) == 4);
    auto r5 = mmap.ReadByte(RegisterType::A);
    TEST_ASSERT(r5.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(r5) == 0xF3);
}
