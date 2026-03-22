#include <cpu/include/instructions.hpp>

// This file exists to improve build times
// The compile-time computation of the instruction registry is quite high, so isolate it to a single TU
#include <cpu/include/instruction.hpp>
#include <cpu/include/instruction_alu.hpp>
#include <cpu/include/instruction_ld.hpp>
#include <cpu/include/instruction_nop.hpp>
#include <cpu/include/instruction_prefix.hpp>
#include <cpu/include/instruction_rsb.hpp>

namespace pgb::cpu
{

// Note that the reference to the registries must be referenced after the instructions are loaded
std::size_t ExecuteActiveDecoder(std::uint_fast8_t opCode, memory::MemoryMap& mmap) noexcept
{
    switch (mmap.GetActivePrefix())
    {
    case 0xCB:
        return instruction::InstructionRegistryPrefixCB::Execute(opCode, mmap);
    [[likely]] default:
        return instruction::InstructionRegistryNoPrefix::Execute(opCode, mmap);
    }
}

std::size_t GetInstructionTicks(std::uint_fast8_t opCode, std::uint_fast8_t prefix) noexcept
{
    switch (prefix)
    {
    case 0xCB:
        return instruction::InstructionRegistryPrefixCB::Ticks[opCode];
    [[likely]] default:
        return instruction::InstructionRegistryNoPrefix::Ticks[opCode];
    }
}

InstructionCallback GetInstructionCallback(std::uint_fast8_t opCode, std::uint_fast8_t prefix) noexcept
{
    switch (prefix)
    {
    case 0xCB:
        return instruction::InstructionRegistryPrefixCB::Callbacks[opCode];
    [[likely]] default:
        return instruction::InstructionRegistryNoPrefix::Callbacks[opCode];
    }
}

} // namespace pgb::cpu
