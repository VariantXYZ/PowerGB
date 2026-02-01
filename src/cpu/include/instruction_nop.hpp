#pragma once

#include <cpu/include/decoder.hpp>
#include <cpu/include/instruction.hpp>

namespace pgb::cpu::instruction
{
using NOP         = Instruction<4, LoadIR>;
using Nop_Decoder = Instantiate<InstructionDecoder<"nop", 0x00, NOP>>;

} // namespace pgb::cpu::instruction
