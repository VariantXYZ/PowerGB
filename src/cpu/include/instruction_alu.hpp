#pragma once

#include <cstdint>

#include <cpu/include/decoder.hpp>
#include <cpu/include/instruction.hpp>
#include <cpu/include/registers.hpp>

using namespace pgb::memory;

namespace pgb::cpu::instruction
{

template <typename T, ResultType... Results>
using AluResultSet =
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

using BasicAluResultSet = AluResultSet<void, memory::MemoryMap::ResultRegisterOverflow>;

template <RegisterType Destination, RegisterType Operand>
    requires(IsRegister8Bit<Destination> && (IsRegister8Bit<Operand> || IsRegister16Bit<Operand>))
inline BasicAluResultSet AddReg(MemoryMap& mmap) noexcept
{
    auto dst = mmap.ReadByte(Destination);
    if (dst.IsFailure())
    {
        return dst;
    }

    Byte srcValue;
    if constexpr (IsRegister8Bit<Operand>)
    {
        // Operand is a value
        auto src = mmap.ReadByte(Operand);
        if (src.IsFailure())
        {
            return src;
        }
        srcValue = static_cast<const Byte>(src);
    }
    else if constexpr (IsRegister16Bit<Operand>)
    {
        // Operand is an address
        auto srcAddr = mmap.ReadWord(Operand);
        if (srcAddr.IsFailure())
        {
            return srcAddr;
        }
        const Word address = static_cast<Word>(srcAddr);
        auto       src     = mmap.ReadByte(static_cast<std::uint_fast16_t>(address));
        if (src.IsFailure())
        {
            return src;
        }
        srcValue = static_cast<const Byte>(src);
    }

    // Note that src value is specifically not a reference
    bool               Z, N, H, C;
    auto&              dstValue    = static_cast<const Byte&>(dst);
    std::uint_fast32_t value       = srcValue + dstValue;
    auto               valueCasted = static_cast<Byte>(value);
    auto               result      = mmap.WriteByte(Destination, static_cast<Byte>(valueCasted));

    // Flags
    Z                              = valueCasted == 0;
    N                              = false;
    H                              = ((dstValue ^ srcValue ^ valueCasted) & 0x10) != 0;
    C                              = value > 0xFF;
    Nibble flag                    = Z << 3 | N << 2 | H << 1 | C << 0;
    mmap.WriteFlag(flag);

    return result;
}

template <RegisterType Destination, RegisterType Operand>
    requires(IsRegister8Bit<Destination> && (IsRegister8Bit<Operand> || IsRegister16Bit<Operand>))
inline BasicAluResultSet SubReg(MemoryMap& mmap) noexcept
{
    auto dst = mmap.ReadByte(Destination);
    if (dst.IsFailure())
    {
        return dst;
    }

    Byte srcValue;
    if constexpr (IsRegister8Bit<Operand>)
    {
        // Operand is a value
        auto src = mmap.ReadByte(Operand);
        if (src.IsFailure())
        {
            return src;
        }
        srcValue = static_cast<const Byte>(src);
    }
    else if constexpr (IsRegister16Bit<Operand>)
    {
        // Operand is an address
        auto srcAddr = mmap.ReadWord(Operand);
        if (srcAddr.IsFailure())
        {
            return srcAddr;
        }
        const Word address = static_cast<Word>(srcAddr);
        auto       src     = mmap.ReadByte(static_cast<std::uint_fast16_t>(address));
        if (src.IsFailure())
        {
            return src;
        }
        srcValue = static_cast<const Byte>(src);
    }

    // Note that src value is specifically not a reference
    bool              Z, N, H, C;
    auto&             dstValue    = static_cast<const Byte&>(dst);
    std::int_fast32_t value       = dstValue - srcValue;
    auto              valueCasted = static_cast<Byte>(value);
    auto              result      = mmap.WriteByte(Destination, static_cast<Byte>(valueCasted));

    // Flags
    Z                             = valueCasted == 0;
    N                             = true;
    H                             = ((dstValue ^ srcValue ^ valueCasted) & 0x10) != 0;
    C                             = value < 0;
    Nibble flag                   = Z << 3 | N << 2 | H << 1 | C << 0;
    mmap.WriteFlag(flag);

    return result;
}

template <auto Destination, auto Operand, std::size_t Ticks = IsRegister16Bit<Operand> ? 8 : 4>
    requires(IsRegister8Bit<Destination> && (IsRegister8Bit<Operand> || IsRegister16Bit<Operand>))
using Add = Instruction<
    /*Ticks*/ Ticks,
    AddReg<Destination, Operand>,
    IncrementPC,
    LoadIRPC>;

template <auto Destination, auto Operand, std::size_t Ticks = IsRegister16Bit<Operand> ? 8 : 4>
    requires(IsRegister8Bit<Destination> && (IsRegister8Bit<Operand> || IsRegister16Bit<Operand>))
using Sub = Instruction<
    /*Ticks*/ Ticks,
    SubReg<Destination, Operand>,
    IncrementPC,
    LoadIRPC>;

using Add_A_B_Decoder          = Instantiate<InstructionDecoder<"add a, b", 0x80, Add<RegisterType::A, RegisterType::B>>>::Type;
using Add_A_C_Decoder          = Instantiate<InstructionDecoder<"add a, c", 0x81, Add<RegisterType::A, RegisterType::C>>>::Type;
using Add_A_D_Decoder          = Instantiate<InstructionDecoder<"add a, d", 0x82, Add<RegisterType::A, RegisterType::D>>>::Type;
using Add_A_E_Decoder          = Instantiate<InstructionDecoder<"add a, e", 0x83, Add<RegisterType::A, RegisterType::E>>>::Type;
using Add_A_H_Decoder          = Instantiate<InstructionDecoder<"add a, h", 0x84, Add<RegisterType::A, RegisterType::H>>>::Type;
using Add_A_L_Decoder          = Instantiate<InstructionDecoder<"add a, l", 0x85, Add<RegisterType::A, RegisterType::L>>>::Type;
using Add_A_IndirectHL_Decoder = Instantiate<InstructionDecoder<"add a, [hl]", 0x86, Add<RegisterType::A, RegisterType::HL>>>::Type;

using Sub_A_B_Decoder          = Instantiate<InstructionDecoder<"sub a, b", 0x90, Sub<RegisterType::A, RegisterType::B>>>::Type;
using Sub_A_C_Decoder          = Instantiate<InstructionDecoder<"sub a, c", 0x91, Sub<RegisterType::A, RegisterType::C>>>::Type;
using Sub_A_D_Decoder          = Instantiate<InstructionDecoder<"sub a, d", 0x92, Sub<RegisterType::A, RegisterType::D>>>::Type;
using Sub_A_E_Decoder          = Instantiate<InstructionDecoder<"sub a, e", 0x93, Sub<RegisterType::A, RegisterType::E>>>::Type;
using Sub_A_H_Decoder          = Instantiate<InstructionDecoder<"sub a, h", 0x94, Sub<RegisterType::A, RegisterType::H>>>::Type;
using Sub_A_L_Decoder          = Instantiate<InstructionDecoder<"sub a, l", 0x95, Sub<RegisterType::A, RegisterType::L>>>::Type;
using Sub_A_IndirectHL_Decoder = Instantiate<InstructionDecoder<"sub a, [hl]", 0x96, Sub<RegisterType::A, RegisterType::HL>>>::Type;

} // namespace pgb::cpu::instruction
