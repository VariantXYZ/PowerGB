#pragma once

#include <cpu/include/decoder.hpp>
#include <cpu/include/instruction.hpp>
#include <cpu/include/registers.hpp>

using namespace pgb::memory;

namespace pgb::cpu::instruction
{

template <typename T, ResultType... Results>
using ControlFlowResultSet =
    common::ResultSet<
        /* Type */ T,
        common::ResultSuccess,
        MemoryMap::ResultAccessInvalidBank,
        MemoryMap::ResultAccessInvalidAddress,
        MemoryMap::ResultAccessProhibitedAddress,
        MemoryMap::ResultAccessReadOnlyProhibitedAddress,
        MemoryMap::ResultAccessCrossesRegionBoundary,
        MemoryMap::ResultAccessRegisterInvalidWidth,
        Results...>;

using BasicControlFlowResultSet = ControlFlowResultSet<void>;

namespace
{

struct PCNoOp
{
    static inline IncrementPCResultSet Execute(memory::MemoryMap&) noexcept
    {
        return IncrementPCResultSet::DefaultResultSuccess();
    }
};

using Jump = Instruction<
    /*Ticks=*/16,
    IncrementPC,
    LoadTempLoPC,
    IncrementPC,
    LoadTempHiPC,
    LoadReg16Temp<RegisterType::PC>,
    PCNoOp, // Don't increment PC since the new address is what we want, but we use IncrementPCResultSet to check the length
    LoadIRPC>;
} // namespace

using JP_Decoder = Instantiate<InstructionDecoder<"jp nnnn", 0xC3, Jump>>::Type;

} // namespace pgb::cpu::instruction
