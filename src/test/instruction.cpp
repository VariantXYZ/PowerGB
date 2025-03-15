#include <common/datatypes.hpp>
#include <common/result.hpp>
#include <cpu/include/instruction.hpp>
#include <cpu/include/registers.hpp>
#include <memory/include/memory.hpp>
#include <test/include/acutest.h>

void test_basic_instruction(void);

TEST_LIST = {
    {"Basic Instruction", test_basic_instruction},

    {NULL, NULL}
};

using namespace pgb;
using namespace pgb::memory;

using Op0ResultSet = memory::MemoryMap::AccessResultSet;
constexpr Op0ResultSet op0(memory::MemoryMap& mmap)
{
    return mmap.WriteByte({0, 0xFFFF}, 0xFF);
}

using Op1ResultSet = common::ResultSet<void, common::ResultSuccess, common::ResultFailure>;
constexpr Op1ResultSet op1(memory::MemoryMap& mmap)
{
    (void)mmap;
    return Op1ResultSet::DefaultResultSuccess();
}

void test_basic_instruction(void)
{
    auto cpu  = std::make_unique<cpu::RegisterFile>();
    auto mmap = std::make_unique<memory::MemoryMap>(*cpu);
    TEST_ASSERT(mmap->Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

    using Instruction1 = cpu::Instruction<op0, op1>;
    static_assert(Instruction1::Cycles == 2);
    auto result0 = Instruction1::ExecuteCycle<0>(*mmap);
    TEST_ASSERT(result0.IsSuccess());
    TEST_ASSERT(result0.IsResult<common::ResultSuccess>());
    auto result1 = Instruction1::ExecuteCycle<1>(*mmap);
    TEST_ASSERT(result1.IsSuccess());
    TEST_ASSERT(result1.IsResult<common::ResultSuccess>());
    auto result = mmap->ReadByte({0, 0xFFFF});
    TEST_ASSERT(static_cast<Byte>(result) == 0xFF);
}