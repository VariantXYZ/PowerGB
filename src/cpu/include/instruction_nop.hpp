#pragma once

#include <cpu/include/instruction.hpp>

namespace pgb::cpu::instruction
{
using NOP = Instruction<NoOp, NoOp, NoOp, NoOp>;
} // namespace pgb::cpu::instruction