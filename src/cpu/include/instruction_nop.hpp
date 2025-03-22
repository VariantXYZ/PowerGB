#pragma once

#include <cpu/include/decoder.hpp>
#include <cpu/include/instruction.hpp>

namespace pgb::cpu::instruction
{
using NOP         = Instruction<NoOp, NoOp, NoOp, NoOp>;
using Nop_Decoder = InstructionDecoder<"nop", 0x00, NOP>;

} // namespace pgb::cpu::instruction