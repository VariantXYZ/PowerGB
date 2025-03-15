#pragma once

#include <type_traits>

#include <common/result.hpp>
#include <cpu/include/instruction.hpp>
#include <cpu/include/registers.hpp>
#include <memory/include/memory.hpp>

using namespace pgb::memory;

namespace pgb::cpu::instruction
{

template <auto V>
concept LdOperand = std::is_same_v<decltype(V), RegisterType> || std::is_same_v<decltype(V), MemoryMap::MemoryAddress>;

template <auto V>
concept LdOperandIs8Bit = !std::is_same_v<decltype(V), RegisterType> || cpu::IsRegister8Bit<V>;

template <typename T>
using LoadResultSet =
    common::ResultSet<
        /* Type */ T,
        MemoryMap::ResultAccessInvalidBank,
        MemoryMap::ResultAccessInvalidAddress,
        MemoryMap::ResultAccessProhibitedAddress,
        MemoryMap::ResultAccessReadOnlyProhibitedAddress,
        MemoryMap::ResultAccessCrossesRegionBoundary,
        MemoryMap::ResultAccessRegisterInvalidWidth>;

// TODO: Detailed result handling
using ResultInstructionLoadRegister8  = LoadResultSet<const Byte>;
using ResultInstructionLoadRegister16 = LoadResultSet<const Word>;

// 8 -> 8
template <auto Destination, auto Source>
    requires(LdOperand<Destination> && LdOperand<Source> && LdOperandIs8Bit<Destination> && LdOperandIs8Bit<Source>)
constexpr ResultInstructionLoadRegister8 Load(MemoryMap& mmap)
{
    auto src = mmap.ReadByte(Source);
    if (src.IsFailure())
    {
        return src;
    }
    return mmap.WriteByte(Destination, static_cast<const Byte>(src));
}

// 16 -> 16
template <auto Destination, auto Source>
    requires(LdOperand<Destination> && LdOperand<Source> && !LdOperandIs8Bit<Destination> && !LdOperandIs8Bit<Source>)
constexpr ResultInstructionLoadRegister16 Load(MemoryMap& mmap)
{
    auto src = mmap.ReadWord(Source);
    if (src.IsFailure())
    {
        return src;
    }
    return mmap.WriteWord(Destination, static_cast<const Word>(src));
}

template <auto Destination, auto Source>
    requires(LdOperand<Destination> && LdOperand<Source>)
using LD = Instruction<
    Load<Destination, Source>,
    NoOp,
    NoOp,
    NoOp>;

} // namespace pgb::cpu::instruction