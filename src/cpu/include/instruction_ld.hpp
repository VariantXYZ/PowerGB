#pragma once

#include <common/result.hpp>
#include <cpu/include/decoder.hpp>
#include <cpu/include/instruction.hpp>
#include <cpu/include/instruction_nop.hpp>
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
constexpr ResultInstructionLoadRegister8 Load(MemoryMap& mmap) noexcept
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
constexpr ResultInstructionLoadRegister16 Load(MemoryMap& mmap) noexcept
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
constexpr ResultInstructionLoadRegisterVoid Load(MemoryMap& mmap) noexcept
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

    return mmap.WriteByte(Destination, static_cast<const Byte>(result));
}

// 8 -> [Reg16]
template <auto Destination, auto Source>
    requires(IsRegister8Bit<Source> && IsRegister16Bit<Destination>)
constexpr ResultInstructionLoadRegisterVoid Load(MemoryMap& mmap) noexcept
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
using LdReg = Instruction<
    /*Ticks*/ 4,
    Load<Destination, Source>,
    LoadIR>;

template <auto Destination, auto Source>
    requires(LdOperand<Destination> && LdOperand<Source>)
using LdMem = Instruction<
    /*Ticks*/ 8,
    Load<Destination, Source>,
    LoadIR>;

// Identity loads
// TODO: Confirm identity loads are actually just NoOps
using Ld_A_A_Decoder  = Instantiate<InstructionDecoder<"ld a, a", 0x7F, NOP>>::Type;
using Ld_D_D_Decoder  = Instantiate<InstructionDecoder<"ld d, d", 0x52, NOP>>::Type;
using Ld_E_E_Decoder  = Instantiate<InstructionDecoder<"ld e, e", 0x5B, NOP>>::Type;
using Ld_B_B_Decoder  = Instantiate<InstructionDecoder<"ld b, b", 0x40, NOP>>::Type;
using Ld_H_H_Decoder  = Instantiate<InstructionDecoder<"ld h, h", 0x64, NOP>>::Type;
using Ld_C_C_Decoder  = Instantiate<InstructionDecoder<"ld c, c", 0x49, NOP>>::Type;
using Ld_L_L_Decoder  = Instantiate<InstructionDecoder<"ld l, l", 0x6D, NOP>>::Type;

// 0 parameter / length 1 instructions
//// A -> [Reg16]
using Ld_BC_A_Decoder = Instantiate<InstructionDecoder<"ld [bc], a", 0x02, LdMem<RegisterType::BC, RegisterType::A>>>::Type;
using Ld_DE_A_Decoder = Instantiate<InstructionDecoder<"ld [de], a", 0x12, LdMem<RegisterType::DE, RegisterType::A>>>::Type;
using Ld_HL_A_Decoder = Instantiate<InstructionDecoder<"ld [hl], a", 0x77, LdMem<RegisterType::HL, RegisterType::A>>>::Type;

//// [Reg16] -> A
using Ld_A_BC_Decoder = Instantiate<InstructionDecoder<"ld a, [bc]", 0x0A, LdMem<RegisterType::A, RegisterType::BC>>>::Type;
using Ld_A_DE_Decoder = Instantiate<InstructionDecoder<"ld a, [de]", 0x1A, LdMem<RegisterType::A, RegisterType::DE>>>::Type;
using Ld_A_HL_Decoder = Instantiate<InstructionDecoder<"ld a, [hl]", 0x7E, LdMem<RegisterType::A, RegisterType::HL>>>::Type;

//// Reg8 -> A
using Ld_A_B_Decoder  = Instantiate<InstructionDecoder<"ld a, b", 0x78, LdReg<RegisterType::A, RegisterType::B>>>::Type;
using Ld_A_C_Decoder  = Instantiate<InstructionDecoder<"ld a, c", 0x79, LdReg<RegisterType::A, RegisterType::C>>>::Type;
using Ld_A_D_Decoder  = Instantiate<InstructionDecoder<"ld a, d", 0x7A, LdReg<RegisterType::A, RegisterType::D>>>::Type;
using Ld_A_E_Decoder  = Instantiate<InstructionDecoder<"ld a, e", 0x7B, LdReg<RegisterType::A, RegisterType::E>>>::Type;
using Ld_A_H_Decoder  = Instantiate<InstructionDecoder<"ld a, h", 0x7C, LdReg<RegisterType::A, RegisterType::H>>>::Type;
using Ld_A_L_Decoder  = Instantiate<InstructionDecoder<"ld a, l", 0x7D, LdReg<RegisterType::A, RegisterType::L>>>::Type;

//// A -> Reg8
using Ld_B_A_Decoder  = Instantiate<InstructionDecoder<"ld b, a", 0x47, LdReg<RegisterType::B, RegisterType::A>>>::Type;
using Ld_C_A_Decoder  = Instantiate<InstructionDecoder<"ld c, a", 0x4F, LdReg<RegisterType::C, RegisterType::A>>>::Type;
using Ld_D_A_Decoder  = Instantiate<InstructionDecoder<"ld d, a", 0x57, LdReg<RegisterType::D, RegisterType::A>>>::Type;
using Ld_E_A_Decoder  = Instantiate<InstructionDecoder<"ld e, a", 0x5F, LdReg<RegisterType::E, RegisterType::A>>>::Type;
using Ld_H_A_Decoder  = Instantiate<InstructionDecoder<"ld h, a", 0x67, LdReg<RegisterType::H, RegisterType::A>>>::Type;
using Ld_L_A_Decoder  = Instantiate<InstructionDecoder<"ld l, a", 0x6F, LdReg<RegisterType::L, RegisterType::A>>>::Type;

} // namespace pgb::cpu::instruction
