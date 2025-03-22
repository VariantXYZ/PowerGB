#pragma once

#include "cpu/include/decoder.hpp"
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

// This must
template <typename T>
using LoadResultSet =
    common::ResultSet<
        /* Type */ T,
        common::ResultSuccess,
        MemoryMap::ResultAccessInvalidBank,
        MemoryMap::ResultAccessInvalidAddress,
        MemoryMap::ResultAccessProhibitedAddress,
        MemoryMap::ResultAccessReadOnlyProhibitedAddress,
        MemoryMap::ResultAccessCrossesRegionBoundary,
        MemoryMap::ResultAccessRegisterInvalidWidth>;

using ResultInstructionLoadRegister8    = LoadResultSet<const Byte>;
using ResultInstructionLoadRegister16   = LoadResultSet<const Word>;
using ResultInstructionLoadRegisterVoid = LoadResultSet<void>;

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

// [Reg16] -> 8
template <auto Destination, auto Source>
    requires(IsRegister8Bit<Destination> && IsRegister16Bit<Source>)
constexpr ResultInstructionLoadRegisterVoid Load(MemoryMap& mmap)
{
    auto src = mmap.ReadWord(Source);
    if (src.IsFailure())
    {
        return src;
    }

    const Word address = static_cast<Word>(src);

    auto result        = mmap.ReadByte(static_cast<uint_fast16_t>(address));
    if (result.IsFailure())
    {
        return result;
    }

    return mmap.WriteByte(Destination, static_cast<const Byte&>(result));
}

// 8 -> [Reg16]
template <auto Destination, auto Source>
    requires(IsRegister8Bit<Source> && IsRegister16Bit<Destination>)
constexpr ResultInstructionLoadRegisterVoid Load(MemoryMap& mmap)
{
    auto src = mmap.ReadByte(Source);
    if (src.IsFailure())
    {
        return src;
    }

    auto dstAddr = mmap.ReadWord(Destination);
    if (dstAddr.IsFailure())
    {
        return dstAddr;
    }
    const Word w = static_cast<Word>(dstAddr);

    return mmap.WriteByte(static_cast<std::uint_fast16_t>(w), static_cast<const Byte>(src));
}

template <auto Destination, auto Source>
    requires(LdOperand<Destination> && LdOperand<Source>)
using Ld = Instruction<
    Load<Destination, Source>,
    NoOp,
    NoOp,
    NoOp>;

// 0 parameter / length 1 instructions
using Ld_BC_A_Decoder = InstructionDecoder<"ld [bc], a", 0x02, Ld<RegisterType::BC, RegisterType::A>>;
using Ld_A_BC_Decoder = InstructionDecoder<"ld a, [bc]", 0x12, Ld<RegisterType::A, RegisterType::BC>>;

} // namespace pgb::cpu::instruction