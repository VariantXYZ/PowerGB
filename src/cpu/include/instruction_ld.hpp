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

using LoadRegisterTempLoResultSet = memory::MemoryMap::BaseRegisterAccessResultSet<void>;
// Z -> A
inline LoadRegisterTempLoResultSet LoadATempLo(memory::MemoryMap& mmap) noexcept
{
    auto result = mmap.WriteByte(RegisterType::A, mmap.GetTempLo());
    return result;
}
// WZ -> Reg16
template <auto Destination>
    requires(IsRegister16Bit<Destination>)
inline LoadRegisterTempLoResultSet LoadReg16Temp(memory::MemoryMap& mmap) noexcept
{
    auto result = mmap.WriteWord(Destination, mmap.GetTemp());
    return result;
}

template <auto Destination, auto Source>
    requires(LdOperand<Destination> && LdOperand<Source>)
using LdReg = Instruction<
    /*Ticks*/ 4,
    Load<Destination, Source>,
    LoadIRPC,
    IncrementPC>;

template <auto Destination, auto Source, IncrementMode Mode = IncrementMode::None>
    requires(LdOperand<Destination> && LdOperand<Source>)
using LdMem = Instruction<
    /*Ticks*/ 8,
    Load<Destination, Source>,
    SingleStepRegister<IsRegister16Bit<Destination> ? Destination : Source, Mode>,
    LoadIRPC,
    IncrementPC>;

template <auto Destination>
    requires(LdOperand<Destination>)
using LdImm = Instruction<
    /*Ticks*/ 12,
    IncrementPC,
    LoadTempLoPC,
    IncrementPC,
    LoadTempHiPC,
    IncrementPC,
    LoadReg16Temp<Destination>,
    LoadIRPC,
    IncrementPC>;

using Ld_BC_Imm_Decoder  = Instantiate<InstructionDecoder<"ld bc, nnnn", 0x01, LdImm<RegisterType::BC>>>::Type;
using Ld_BC_A_Decoder    = Instantiate<InstructionDecoder<"ld [bc], a", 0x02, LdMem<RegisterType::BC, RegisterType::A>>>::Type;

using Ld_A_BC_Decoder    = Instantiate<InstructionDecoder<"ld a, [bc]", 0x0A, LdMem<RegisterType::A, RegisterType::BC>>>::Type;
using Ld_DE_Imm_Decoder  = Instantiate<InstructionDecoder<"ld de, nnnn", 0x11, LdImm<RegisterType::DE>>>::Type;
using Ld_DE_A_Decoder    = Instantiate<InstructionDecoder<"ld [de], a", 0x12, LdMem<RegisterType::DE, RegisterType::A>>>::Type;

using Ld_A_DE_Decoder    = Instantiate<InstructionDecoder<"ld a, [de]", 0x1A, LdMem<RegisterType::A, RegisterType::DE>>>::Type;
using Ld_HL_Imm_Decoder  = Instantiate<InstructionDecoder<"ld hl, nnnn", 0x21, LdImm<RegisterType::HL>>>::Type;
using Ld_A_HLInc_Decoder = Instantiate<InstructionDecoder<"ld [hli], a", 0x22, LdMem<RegisterType::HL, RegisterType::A, IncrementMode::Increment>>>::Type;
using Ld_HLInc_A_Decoder = Instantiate<InstructionDecoder<"ld a, [hli]", 0x2A, LdMem<RegisterType::A, RegisterType::HL, IncrementMode::Increment>>>::Type;
using Ld_SP_Imm_Decoder  = Instantiate<InstructionDecoder<"ld sp, nnnn", 0x31, LdImm<RegisterType::SP>>>::Type;
using Ld_A_HLDec_Decoder = Instantiate<InstructionDecoder<"ld [hld], a", 0x32, LdMem<RegisterType::HL, RegisterType::A, IncrementMode::Decrement>>>::Type;
using Ld_HLDec_A_Decoder = Instantiate<InstructionDecoder<"ld a, [hld]", 0x3A, LdMem<RegisterType::A, RegisterType::HL, IncrementMode::Decrement>>>::Type;
using Ld_B_B_Decoder     = Instantiate<InstructionDecoder<"ld b, b", 0x40, NOP>>::Type;
using Ld_B_C_Decoder     = Instantiate<InstructionDecoder<"ld b, c", 0x41, LdReg<RegisterType::B, RegisterType::C>>>::Type;
using Ld_B_D_Decoder     = Instantiate<InstructionDecoder<"ld b, d", 0x42, LdReg<RegisterType::B, RegisterType::D>>>::Type;
using Ld_B_E_Decoder     = Instantiate<InstructionDecoder<"ld b, e", 0x43, LdReg<RegisterType::B, RegisterType::E>>>::Type;
using Ld_B_H_Decoder     = Instantiate<InstructionDecoder<"ld b, h", 0x44, LdReg<RegisterType::B, RegisterType::H>>>::Type;
using Ld_B_L_Decoder     = Instantiate<InstructionDecoder<"ld b, l", 0x45, LdReg<RegisterType::B, RegisterType::L>>>::Type;
using Ld_B_HL_Decoder    = Instantiate<InstructionDecoder<"ld b, [hl]", 0x46, LdReg<RegisterType::B, RegisterType::HL>>>::Type;
using Ld_B_A_Decoder     = Instantiate<InstructionDecoder<"ld b, a", 0x47, LdReg<RegisterType::B, RegisterType::A>>>::Type;
using Ld_C_B_Decoder     = Instantiate<InstructionDecoder<"ld c, b", 0x48, LdReg<RegisterType::C, RegisterType::B>>>::Type;
using Ld_C_C_Decoder     = Instantiate<InstructionDecoder<"ld c, c", 0x49, NOP>>::Type;
using Ld_C_D_Decoder     = Instantiate<InstructionDecoder<"ld c, d", 0x4A, LdReg<RegisterType::C, RegisterType::D>>>::Type;
using Ld_C_E_Decoder     = Instantiate<InstructionDecoder<"ld c, e", 0x4B, LdReg<RegisterType::C, RegisterType::E>>>::Type;
using Ld_C_H_Decoder     = Instantiate<InstructionDecoder<"ld c, h", 0x4C, LdReg<RegisterType::C, RegisterType::H>>>::Type;
using Ld_C_L_Decoder     = Instantiate<InstructionDecoder<"ld c, l", 0x4D, LdReg<RegisterType::C, RegisterType::L>>>::Type;
using Ld_C_HL_Decoder    = Instantiate<InstructionDecoder<"ld c, [hl]", 0x4E, LdReg<RegisterType::C, RegisterType::HL>>>::Type;
using Ld_C_A_Decoder     = Instantiate<InstructionDecoder<"ld c, a", 0x4F, LdReg<RegisterType::C, RegisterType::A>>>::Type;

using Ld_D_B_Decoder     = Instantiate<InstructionDecoder<"ld d, b", 0x50, LdReg<RegisterType::D, RegisterType::B>>>::Type;
using Ld_D_C_Decoder     = Instantiate<InstructionDecoder<"ld d, c", 0x51, LdReg<RegisterType::D, RegisterType::C>>>::Type;
using Ld_D_D_Decoder     = Instantiate<InstructionDecoder<"ld d, d", 0x52, NOP>>::Type;
using Ld_D_E_Decoder     = Instantiate<InstructionDecoder<"ld d, e", 0x53, LdReg<RegisterType::D, RegisterType::E>>>::Type;
using Ld_D_H_Decoder     = Instantiate<InstructionDecoder<"ld d, h", 0x54, LdReg<RegisterType::D, RegisterType::H>>>::Type;
using Ld_D_L_Decoder     = Instantiate<InstructionDecoder<"ld d, l", 0x55, LdReg<RegisterType::D, RegisterType::L>>>::Type;
using Ld_D_HL_Decoder    = Instantiate<InstructionDecoder<"ld d, [hl]", 0x56, LdReg<RegisterType::D, RegisterType::HL>>>::Type;
using Ld_D_A_Decoder     = Instantiate<InstructionDecoder<"ld d, a", 0x57, LdReg<RegisterType::D, RegisterType::A>>>::Type;
using Ld_E_B_Decoder     = Instantiate<InstructionDecoder<"ld e, b", 0x58, LdReg<RegisterType::E, RegisterType::B>>>::Type;
using Ld_E_C_Decoder     = Instantiate<InstructionDecoder<"ld e, c", 0x59, LdReg<RegisterType::E, RegisterType::C>>>::Type;
using Ld_E_D_Decoder     = Instantiate<InstructionDecoder<"ld e, d", 0x5A, LdReg<RegisterType::E, RegisterType::D>>>::Type;
using Ld_E_E_Decoder     = Instantiate<InstructionDecoder<"ld e, e", 0x5B, NOP>>::Type;
using Ld_E_H_Decoder     = Instantiate<InstructionDecoder<"ld e, h", 0x5C, LdReg<RegisterType::E, RegisterType::H>>>::Type;
using Ld_E_L_Decoder     = Instantiate<InstructionDecoder<"ld e, l", 0x5D, LdReg<RegisterType::E, RegisterType::L>>>::Type;
using Ld_E_HL_Decoder    = Instantiate<InstructionDecoder<"ld e, [hl]", 0x5E, LdReg<RegisterType::E, RegisterType::HL>>>::Type;
using Ld_E_A_Decoder     = Instantiate<InstructionDecoder<"ld e, a", 0x5F, LdReg<RegisterType::E, RegisterType::A>>>::Type;

using Ld_H_B_Decoder     = Instantiate<InstructionDecoder<"ld h, b", 0x60, LdReg<RegisterType::H, RegisterType::B>>>::Type;
using Ld_H_C_Decoder     = Instantiate<InstructionDecoder<"ld h, c", 0x61, LdReg<RegisterType::H, RegisterType::C>>>::Type;
using Ld_H_D_Decoder     = Instantiate<InstructionDecoder<"ld h, d", 0x62, LdReg<RegisterType::H, RegisterType::D>>>::Type;
using Ld_H_E_Decoder     = Instantiate<InstructionDecoder<"ld h, e", 0x63, LdReg<RegisterType::H, RegisterType::E>>>::Type;
using Ld_H_H_Decoder     = Instantiate<InstructionDecoder<"ld h, h", 0x64, NOP>>::Type;
using Ld_H_L_Decoder     = Instantiate<InstructionDecoder<"ld h, l", 0x65, LdReg<RegisterType::H, RegisterType::L>>>::Type;
using Ld_H_HL_Decoder    = Instantiate<InstructionDecoder<"ld h, [hl]", 0x66, LdReg<RegisterType::H, RegisterType::HL>>>::Type;
using Ld_H_A_Decoder     = Instantiate<InstructionDecoder<"ld h, a", 0x67, LdReg<RegisterType::H, RegisterType::A>>>::Type;
using Ld_L_B_Decoder     = Instantiate<InstructionDecoder<"ld l, b", 0x68, LdReg<RegisterType::L, RegisterType::B>>>::Type;
using Ld_L_C_Decoder     = Instantiate<InstructionDecoder<"ld l, c", 0x69, LdReg<RegisterType::L, RegisterType::C>>>::Type;
using Ld_L_D_Decoder     = Instantiate<InstructionDecoder<"ld l, d", 0x6A, LdReg<RegisterType::L, RegisterType::D>>>::Type;
using Ld_L_E_Decoder     = Instantiate<InstructionDecoder<"ld l, e", 0x6B, LdReg<RegisterType::L, RegisterType::E>>>::Type;
using Ld_L_H_Decoder     = Instantiate<InstructionDecoder<"ld l, h", 0x6C, LdReg<RegisterType::L, RegisterType::H>>>::Type;
using Ld_L_L_Decoder     = Instantiate<InstructionDecoder<"ld l, l", 0x6D, NOP>>::Type;
using Ld_L_HL_Decoder    = Instantiate<InstructionDecoder<"ld l, [hl]", 0x6E, LdReg<RegisterType::L, RegisterType::HL>>>::Type;
using Ld_L_A_Decoder     = Instantiate<InstructionDecoder<"ld l, a", 0x6F, LdReg<RegisterType::L, RegisterType::A>>>::Type;

using Ld_HL_B_Decoder    = Instantiate<InstructionDecoder<"ld [hl], b", 0x70, LdMem<RegisterType::HL, RegisterType::B>>>::Type;
using Ld_HL_C_Decoder    = Instantiate<InstructionDecoder<"ld [hl], c", 0x71, LdMem<RegisterType::HL, RegisterType::C>>>::Type;
using Ld_HL_D_Decoder    = Instantiate<InstructionDecoder<"ld [hl], d", 0x72, LdMem<RegisterType::HL, RegisterType::D>>>::Type;
using Ld_HL_E_Decoder    = Instantiate<InstructionDecoder<"ld [hl], e", 0x73, LdMem<RegisterType::HL, RegisterType::E>>>::Type;
using Ld_HL_H_Decoder    = Instantiate<InstructionDecoder<"ld [hl], h", 0x74, LdMem<RegisterType::HL, RegisterType::H>>>::Type;
using Ld_HL_L_Decoder    = Instantiate<InstructionDecoder<"ld [hl], l", 0x75, LdMem<RegisterType::HL, RegisterType::L>>>::Type;
// TODO: 0x76 ld [hl], [hl] is 'HALT'
using Ld_HL_A_Decoder    = Instantiate<InstructionDecoder<"ld [hl], a", 0x77, LdMem<RegisterType::HL, RegisterType::A>>>::Type;
using Ld_A_B_Decoder     = Instantiate<InstructionDecoder<"ld a, b", 0x78, LdReg<RegisterType::A, RegisterType::B>>>::Type;
using Ld_A_C_Decoder     = Instantiate<InstructionDecoder<"ld a, c", 0x79, LdReg<RegisterType::A, RegisterType::C>>>::Type;
using Ld_A_D_Decoder     = Instantiate<InstructionDecoder<"ld a, d", 0x7A, LdReg<RegisterType::A, RegisterType::D>>>::Type;
using Ld_A_E_Decoder     = Instantiate<InstructionDecoder<"ld a, e", 0x7B, LdReg<RegisterType::A, RegisterType::E>>>::Type;
using Ld_A_H_Decoder     = Instantiate<InstructionDecoder<"ld a, h", 0x7C, LdReg<RegisterType::A, RegisterType::H>>>::Type;
using Ld_A_L_Decoder     = Instantiate<InstructionDecoder<"ld a, l", 0x7D, LdReg<RegisterType::A, RegisterType::L>>>::Type;
using Ld_A_HL_Decoder    = Instantiate<InstructionDecoder<"ld a, [hl]", 0x7E, LdMem<RegisterType::A, RegisterType::HL>>>::Type;
using Ld_A_A_Decoder     = Instantiate<InstructionDecoder<"ld a, a", 0x7F, NOP>>::Type;

} // namespace pgb::cpu::instruction
