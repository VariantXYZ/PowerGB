#pragma once

#include <cpu/include/decoder.hpp>
#include <cpu/include/instruction.hpp>

namespace pgb::cpu::instruction
{
using NOP         = Instruction<4, LoadIRPC>;
using Nop_Decoder = Instantiate<InstructionDecoder<"nop", 0x00, NOP>>::Type;

} // namespace pgb::cpu::instruction
