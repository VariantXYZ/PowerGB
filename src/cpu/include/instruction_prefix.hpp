#pragma once

#include <common/result.hpp>
#include <cpu/include/instruction.hpp>
#include <memory/include/memory.hpp>

using namespace pgb::memory;

namespace pgb::cpu::instruction
{

using PrefixResultSet = common::ResultSet<void, common::ResultSuccess, common::ResultFailure>;

template <Byte Prefix>
inline constexpr PrefixResultSet HandlePrefix(memory::MemoryMap& mmap) noexcept
{
    mmap.SetActivePrefix(Prefix);
    return PrefixResultSet::DefaultResultSuccess();
}

using PrefixCB = Instruction<
    /*Ticks*/ 4,
    HandlePrefix<0xCB>,
    IncrementPC,
    LoadIRPC>;

} // namespace pgb::cpu::instruction
