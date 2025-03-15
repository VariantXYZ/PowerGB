#include <common/datatypes.hpp>
#include <common/result.hpp>
#include <cpu/include/instruction.hpp>
#include <cpu/include/registers.hpp>
#include <memory/include/memory.hpp>
#include <test/include/acutest.h>

void test_basic_instruction_template(void);
void test_basic_instruction_index(void);
void test_execute_failure(void);
void test_execute_all(void);

TEST_LIST = {
    {"Basic Instruction (operations via template parameter)", test_basic_instruction_template},
    {"Basic Instruction (operations via runtime index)", test_basic_instruction_index},
    {"Execute All", test_execute_all},
    {"Execute until failure", test_execute_failure},
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

using Op2ResultSet = common::ResultSet<void, common::ResultSuccess, common::ResultFailure>;
constexpr Op1ResultSet op2(memory::MemoryMap& mmap)
{
    (void)mmap;
    return Op1ResultSet::DefaultResultFailure();
}

static auto registers = cpu::RegisterFile();
static auto mmap      = MemoryMap(registers, MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount);

void test_basic_instruction_template(void)
{
    mmap.Reset();
    TEST_ASSERT(mmap.Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

    using Instruction1 = cpu::Instruction<op0, op1>;
    static_assert(Instruction1::Ticks == 2);
    auto result0 = Instruction1::ExecuteCycle<0>(mmap);
    TEST_ASSERT(result0.IsSuccess());
    TEST_ASSERT(result0.IsResult<common::ResultSuccess>());
    auto result1 = Instruction1::ExecuteCycle<1>(mmap);
    TEST_ASSERT(result1.IsSuccess());
    TEST_ASSERT(result1.IsResult<common::ResultSuccess>());
    auto result = mmap.ReadByte({0, 0xFFFF});
    TEST_ASSERT(static_cast<Byte>(result) == 0xFF);
}

void test_basic_instruction_index(void)
{
    mmap.Reset();
    TEST_ASSERT(mmap.Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

    using Instruction1 = cpu::Instruction<op0, op1>;
    static_assert(Instruction1::Ticks == 2);
    auto result0 = Instruction1::ExecuteCycle(mmap, 0);
    TEST_ASSERT(result0);
    auto result1 = Instruction1::ExecuteCycle(mmap, 0);
    TEST_ASSERT(result1);
    auto result = mmap.ReadByte({0, 0xFFFF});
    TEST_ASSERT(static_cast<Byte>(result) == 0xFF);
}

void test_execute_all(void)
{
    using Instruction0 = cpu::Instruction<op0>;
    static_assert(Instruction0::Ticks == 1);
    std::size_t ticks = Instruction0::ExecuteAll(mmap);
    Byte        value = mmap.ReadByte({0, 0xFFFF});
    TEST_ASSERT(ticks == Instruction0::Ticks);
    TEST_ASSERT(value == 0xFF);
}

void test_execute_failure(void)
{
    mmap.Reset();
    TEST_ASSERT(mmap.Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());
    {
        using Instruction0 = cpu::Instruction<op2, op1, op0>;
        static_assert(Instruction0::Ticks == 3);
        auto ticks = Instruction0::ExecuteAll(mmap);
        TEST_ASSERT(ticks == 0);
    }

    {
        using Instruction0 = cpu::Instruction<op1, op2, op0>;
        static_assert(Instruction0::Ticks == 3);
        auto ticks = Instruction0::ExecuteAll(mmap);
        TEST_ASSERT(ticks == 1);
    }
}
