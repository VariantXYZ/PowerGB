#pragma once

#include <cstdint>

#include <common/util.hpp>
#include <cpu/include/instruction.hpp>
#include <memory/include/memory.hpp>

// There are only a few types of instructions to decode:
// * 0 parameters
// * 1 parameter (16 bit)
// * 1 parameter (8 bit)
// * 2 parameters (8, 8)
// * 2 parameters (8, Reg16)
// * 2 parameters (Reg16, Reg16)

using namespace pgb::common;

namespace pgb::cpu::instruction
{

// 0 parameter instructions
template <util::StringLiteral Name, std::uint_fast8_t OpCode, InstructionType InstructionHandler>
class InstructionDecoder
{
    InstructionDecoder()              = delete;

    constexpr static auto Instruction = Name.value;
    constexpr static auto Bytes       = OpCode;

    constexpr static std::size_t Execute(memory::MemoryMap& mmap)
    {
        return InstructionHandler::ExecuteAll(mmap);
    }
};

} // namespace pgb::cpu::instruction