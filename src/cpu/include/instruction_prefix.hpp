#pragma once

#include <common/result.hpp>
#include <cpu/include/decoder.hpp>
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

template <Byte P>
using Prefix = Instruction<
    /*Ticks*/ 4,
    HandlePrefix<P>,
    IncrementPC,
    LoadIRPC>;

using Prefix_CB_Decoder = Instantiate<InstructionDecoder<"prefix cb", 0xCB, Prefix<0xCB>>>::Type;

} // namespace pgb::cpu::instruction
