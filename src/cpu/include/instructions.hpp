#pragma once

#include <cstdint>

#include <memory/include/memory.hpp>

namespace pgb::cpu
{

using InstructionCallback = std::size_t (*)(memory::MemoryMap&) noexcept;

std::size_t         GetInstructionTicks(std::uint_fast8_t opCode, std::uint_fast8_t prefix = 0x00) noexcept;
InstructionCallback GetInstructionCallback(std::uint_fast8_t opCode, std::uint_fast8_t prefix = 0x00) noexcept;

std::size_t ExecuteActiveDecoder(std::uint_fast8_t opCode, memory::MemoryMap& mmap) noexcept;

} // namespace pgb::cpu
