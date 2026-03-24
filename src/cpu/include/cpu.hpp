#pragma once

#include <cstddef>

#include <cpu/include/instruction_alu.hpp>
#include <cpu/include/instruction_ld.hpp>
#include <cpu/include/instruction_nop.hpp>
#include <cpu/include/instruction_prefix.hpp>
#include <cpu/include/instruction_rsb.hpp>

namespace pgb::cpu
{

// Note that the reference to the registries must be referenced after the instructions are loaded
inline constexpr std::size_t ExecuteActiveDecoder(std::uint_fast8_t opCode, memory::MemoryMap& mmap) noexcept
{
    switch (mmap.GetActivePrefix())
    {
    case 0xCB:
        return instruction::InstructionRegistryPrefixCB::Execute(opCode, mmap);
    [[likely]] default:
        return instruction::InstructionRegistryNoPrefix::Execute(opCode, mmap);
    }
}

} // namespace pgb::cpu
