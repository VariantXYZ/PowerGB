#include <common/datatypes.hpp>
#include <cpu/include/instruction.hpp>
#include <cpu/include/registers.hpp>
#include <memory/include/memory.hpp>

#include <test/include/acutest.h>

TEST_LIST = {
    {NULL, NULL},
};

using namespace pgb;
using namespace pgb::memory;

using Op0ResultSet = memory::MemoryMap::AccessResultSet;
constexpr Op0ResultSet op0(memory::MemoryMap& mmap)
{
    return mmap.WriteByte({0, 0xFFFF}, 0xFF);
}

consteval Byte SetAndCheckIEValue()
{
    auto registers = cpu::RegisterFile();
    auto mmap      = MemoryMap(registers);
    mmap.Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount);

    using Instruction0 = cpu::Instruction<op0>;
    static_assert(Instruction0::Cycles == 1);
    Instruction0::ExecuteAll(mmap);

    return mmap.ReadByte({0, 0xFFFF});
}
static_assert(SetAndCheckIEValue() == 0xFF);