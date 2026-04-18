#pragma once

#include <cpu/include/decoder.hpp>
#include <cpu/include/instruction.hpp>
#include <cpu/include/registers.hpp>

#include <cstdint>

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

enum Condition
{
    Z,
    C,
    NZ,
    NC,
};

struct PCNoOp
{
    static inline IncrementPCResultSet Execute(memory::MemoryMap&) noexcept
    {
        return IncrementPCResultSet::DefaultResultSuccess();
    }
};

template <Condition Cond, Operation OpSuccess, Operation OpFail>
struct ConditionalPCOperation
{
    // Note that we return IncrementPCResultSet specifically here
    static inline IncrementPCResultSet Execute(memory::MemoryMap& memory) noexcept
    {
        bool cond;
        switch (Cond)
        {
        case Z:
        case NZ:
            cond = memory.ReadFlagBit(memory::MemoryMap::FlagBit::Zero);
            break;
        case C:
        case NC:
            cond = memory.ReadFlagBit(memory::MemoryMap::FlagBit::Carry);
            break;
        default:
            break;
        }

        if constexpr (Cond == NZ || Cond == NC)
        {
            cond = !cond;
        }

        // TODO: Failure cases should be propagated through the result failure
        if (cond)
        {
            auto result = OpSuccess::Execute(memory);
            if (result.IsFailure())
            {
                IncrementPCResultSet::DefaultResultFailure();
            }
        }
        else
        {
            memory.SetAlternativeTicks();

            auto result = OpFail::Execute(memory);
            if (result.IsFailure())
            {
                IncrementPCResultSet::DefaultResultFailure();
            }
        }

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

template <Condition Cond, std::size_t TicksSuccess, std::size_t TicksFail>
using JumpCond = Instruction<
    /*Ticks=*/{TicksSuccess, TicksFail},
    IncrementPC,
    LoadTempLoPC,
    IncrementPC,
    LoadTempHiPC,
    ConditionalPCOperation<Cond, LoadReg16Temp<RegisterType::PC>, IncrementPC>,
    LoadIRPC>;

} // namespace

using JP_NZ_Decoder = Instantiate<InstructionDecoder<"jp NZ, nnnn", 0xC2, JumpCond<NZ, 16, 12>>>::Type;
using JP_Z_Decoder = Instantiate<InstructionDecoder<"jp Z, nnnn", 0xCA, JumpCond<Z, 16, 12>>>::Type;
using JP_Decoder   = Instantiate<InstructionDecoder<"jp nnnn", 0xC3, Jump>>::Type;
using JP_NC_Decoder = Instantiate<InstructionDecoder<"jp NC, nnnn", 0xD2, JumpCond<NC, 16, 12>>>::Type;
using JP_C_Decoder = Instantiate<InstructionDecoder<"jp C, nnnn", 0xDA, JumpCond<C, 16, 12>>>::Type;

} // namespace pgb::cpu::instruction
