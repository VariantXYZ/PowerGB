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

using BasicAluResultSet             = AluResultSet<void, memory::MemoryMap::ResultRegisterOverflow>;
using BasicAluOperationCallback     = std::int_fast32_t (*)(Byte /*Destination*/, Byte /*Operand*/, bool /*Carry bit*/);
using BasicAluOperationFlagCallback = Nibble (*)(std::int_fast32_t /*Result*/, Byte /*Destination*/, Byte /*Operand*/);

template <RegisterType Destination, RegisterType Operand, auto Operation, auto Flag>
    requires(IsRegister8Bit<Destination> && (IsRegister8Bit<Operand> || IsRegister16Bit<Operand>))
inline BasicAluResultSet BasicAluOperation(MemoryMap& mmap) noexcept
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
    auto&              dstValue    = static_cast<const Byte&>(dst);
    std::uint_fast32_t value       = Operation(dstValue, srcValue, mmap.ReadFlag());
    auto               valueCasted = static_cast<Byte>(value);
    auto               result      = mmap.WriteByte(Destination, static_cast<Byte>(valueCasted));

    mmap.WriteFlag(Flag(value, dstValue, srcValue));

    return result;
}

template <RegisterType Destination, RegisterType Operand, bool Carry>
inline BasicAluResultSet AddReg(MemoryMap& mmap) noexcept
{
    return BasicAluOperation<Destination, Operand, [](Byte dstValue, Byte srcValue, Nibble flag)
                             {
                                 if constexpr (Carry)
                                 {
                                     return dstValue + srcValue + (flag & 0x1);
                                 }
                                 else
                                 {
                                     return dstValue + srcValue;
                                 } },
                             [](std::int_fast32_t value, Byte dstValue, Byte srcValue)
                             {
                                 auto valueCasted = static_cast<Byte>(value);
                                 // Flags
                                 bool Z           = valueCasted == 0;
                                 bool N           = false;
                                 bool H           = ((dstValue ^ srcValue ^ valueCasted) & 0x10) != 0;
                                 bool C           = value > 0xFF;
                                 return Z << 3 | N << 2 | H << 1 | C << 0;
                             }>(mmap);
}

template <RegisterType Destination, RegisterType Operand, bool Carry>
inline BasicAluResultSet SubReg(MemoryMap& mmap) noexcept
{
    return BasicAluOperation<Destination, Operand, [](Byte dstValue, Byte srcValue, Nibble flag)
                             {
                                 if constexpr (Carry)
                                 {
                                     return dstValue - srcValue - (flag & 0x1);
                                 }
                                 else
                                 {
                                     return dstValue - srcValue;
                                 } },
                             [](std::int_fast32_t value, Byte dstValue, Byte srcValue)
                             {
                                 auto valueCasted = static_cast<Byte>(value);
                                 // Flags
                                 bool Z           = valueCasted == 0;
                                 bool N           = true;
                                 bool H           = ((dstValue ^ srcValue ^ valueCasted) & 0x10) != 0;
                                 bool C           = value < 0;
                                 return Z << 3 | N << 2 | H << 1 | C << 0;
                             }>(mmap);
}

template <RegisterType Destination, RegisterType Operand>
inline BasicAluResultSet CpReg(MemoryMap& mmap) noexcept
{
    // Sub but without any non-flag writes
    return BasicAluOperation<Destination, Operand, [](Byte dstValue, Byte /*unused*/, Nibble /*unused*/)
                             { return dstValue; },
                             [](std::int_fast32_t /*unused*/, Byte dstValue, Byte srcValue)
                             {
                                 auto value       = dstValue - srcValue;
                                 auto valueCasted = static_cast<Byte>(value);
                                 // Flags
                                 bool Z           = valueCasted == 0;
                                 bool N           = true;
                                 bool H           = ((dstValue ^ srcValue ^ valueCasted) & 0x10) != 0;
                                 bool C           = value < 0;
                                 return Z << 3 | N << 2 | H << 1 | C << 0;
                             }>(mmap);
}

template <RegisterType Destination, RegisterType Operand>
inline BasicAluResultSet AndReg(MemoryMap& mmap) noexcept
{
    return BasicAluOperation<Destination, Operand, [](Byte dstValue, Byte srcValue, Nibble /*unused*/)
                             { return dstValue & srcValue; },
                             [](std::int_fast32_t value, Byte /*unused*/, Byte /*unused*/)
                             {
                                 auto valueCasted = static_cast<Byte>(value);
                                 // Flags
                                 bool Z           = valueCasted == 0;
                                 bool N           = false;
                                 bool H           = true;
                                 bool C           = false;
                                 return Z << 3 | N << 2 | H << 1 | C << 0;
                             }>(mmap);
}

template <RegisterType Destination, RegisterType Operand>
inline BasicAluResultSet XorReg(MemoryMap& mmap) noexcept
{
    return BasicAluOperation<Destination, Operand, [](Byte dstValue, Byte srcValue, Nibble /*unused*/)
                             { return dstValue ^ srcValue; },
                             [](std::int_fast32_t value, Byte /*unused*/, Byte /*unused*/)
                             {
                                 auto valueCasted = static_cast<Byte>(value);
                                 // Flags
                                 bool Z           = valueCasted == 0;
                                 bool N           = false;
                                 bool H           = false;
                                 bool C           = false;
                                 return Z << 3 | N << 2 | H << 1 | C << 0;
                             }>(mmap);
}

template <RegisterType Destination, RegisterType Operand>
inline BasicAluResultSet OrReg(MemoryMap& mmap) noexcept
{
    return BasicAluOperation<Destination, Operand, [](Byte dstValue, Byte srcValue, Nibble /*unused*/)
                             { return dstValue | srcValue; },
                             [](std::int_fast32_t value, Byte /*unused*/, Byte /*unused*/)
                             {
                                 auto valueCasted = static_cast<Byte>(value);
                                 // Flags
                                 bool Z           = valueCasted == 0;
                                 bool N           = false;
                                 bool H           = false;
                                 bool C           = false;
                                 return Z << 3 | N << 2 | H << 1 | C << 0;
                             }>(mmap);
}

template <auto Destination, auto Operand, std::size_t Ticks = IsRegister16Bit<Operand> ? 8 : 4>
    requires(IsRegister8Bit<Destination> && (IsRegister8Bit<Operand> || IsRegister16Bit<Operand>))
using Add = Instruction<
    /*Ticks*/ Ticks,
    AddReg<Destination, Operand, false>,
    IncrementPC,
    LoadIRPC>;

template <auto Destination, auto Operand, std::size_t Ticks = IsRegister16Bit<Operand> ? 8 : 4>
    requires(IsRegister8Bit<Destination> && (IsRegister8Bit<Operand> || IsRegister16Bit<Operand>))
using Adc = Instruction<
    /*Ticks*/ Ticks,
    AddReg<Destination, Operand, true>,
    IncrementPC,
    LoadIRPC>;

template <auto Destination, auto Operand, std::size_t Ticks = IsRegister16Bit<Operand> ? 8 : 4>
    requires(IsRegister8Bit<Destination> && (IsRegister8Bit<Operand> || IsRegister16Bit<Operand>))
using Sub = Instruction<
    /*Ticks*/ Ticks,
    SubReg<Destination, Operand, false>,
    IncrementPC,
    LoadIRPC>;

template <auto Destination, auto Operand, std::size_t Ticks = IsRegister16Bit<Operand> ? 8 : 4>
    requires(IsRegister8Bit<Destination> && (IsRegister8Bit<Operand> || IsRegister16Bit<Operand>))
using Sbc = Instruction<
    /*Ticks*/ Ticks,
    SubReg<Destination, Operand, true>,
    IncrementPC,
    LoadIRPC>;

template <auto Destination, auto Operand, std::size_t Ticks = IsRegister16Bit<Operand> ? 8 : 4>
    requires(IsRegister8Bit<Destination> && (IsRegister8Bit<Operand> || IsRegister16Bit<Operand>))
using And = Instruction<
    /*Ticks*/ Ticks,
    AndReg<Destination, Operand>,
    IncrementPC,
    LoadIRPC>;

template <auto Destination, auto Operand, std::size_t Ticks = IsRegister16Bit<Operand> ? 8 : 4>
    requires(IsRegister8Bit<Destination> && (IsRegister8Bit<Operand> || IsRegister16Bit<Operand>))
using Xor = Instruction<
    /*Ticks*/ Ticks,
    XorReg<Destination, Operand>,
    IncrementPC,
    LoadIRPC>;

template <auto Destination, auto Operand, std::size_t Ticks = IsRegister16Bit<Operand> ? 8 : 4>
    requires(IsRegister8Bit<Destination> && (IsRegister8Bit<Operand> || IsRegister16Bit<Operand>))
using Or = Instruction<
    /*Ticks*/ Ticks,
    OrReg<Destination, Operand>,
    IncrementPC,
    LoadIRPC>;

template <auto Destination, auto Operand, std::size_t Ticks = IsRegister16Bit<Operand> ? 8 : 4>
    requires(IsRegister8Bit<Destination> && (IsRegister8Bit<Operand> || IsRegister16Bit<Operand>))
using Cp = Instruction<
    /*Ticks*/ Ticks,
    CpReg<Destination, Operand>,
    IncrementPC,
    LoadIRPC>;

using Add_A_B_Decoder          = Instantiate<InstructionDecoder<"add a, b", 0x80, Add<RegisterType::A, RegisterType::B>>>::Type;
using Add_A_C_Decoder          = Instantiate<InstructionDecoder<"add a, c", 0x81, Add<RegisterType::A, RegisterType::C>>>::Type;
using Add_A_D_Decoder          = Instantiate<InstructionDecoder<"add a, d", 0x82, Add<RegisterType::A, RegisterType::D>>>::Type;
using Add_A_E_Decoder          = Instantiate<InstructionDecoder<"add a, e", 0x83, Add<RegisterType::A, RegisterType::E>>>::Type;
using Add_A_H_Decoder          = Instantiate<InstructionDecoder<"add a, h", 0x84, Add<RegisterType::A, RegisterType::H>>>::Type;
using Add_A_L_Decoder          = Instantiate<InstructionDecoder<"add a, l", 0x85, Add<RegisterType::A, RegisterType::L>>>::Type;
using Add_A_IndirectHL_Decoder = Instantiate<InstructionDecoder<"add a, [hl]", 0x86, Add<RegisterType::A, RegisterType::HL>>>::Type;
using Add_A_A_Decoder          = Instantiate<InstructionDecoder<"add a, a", 0x87, Add<RegisterType::A, RegisterType::A>>>::Type;

using Adc_A_B_Decoder          = Instantiate<InstructionDecoder<"adc a, b", 0x88, Adc<RegisterType::A, RegisterType::B>>>::Type;
using Adc_A_C_Decoder          = Instantiate<InstructionDecoder<"adc a, c", 0x89, Adc<RegisterType::A, RegisterType::C>>>::Type;
using Adc_A_D_Decoder          = Instantiate<InstructionDecoder<"adc a, d", 0x8A, Adc<RegisterType::A, RegisterType::D>>>::Type;
using Adc_A_E_Decoder          = Instantiate<InstructionDecoder<"adc a, e", 0x8B, Adc<RegisterType::A, RegisterType::E>>>::Type;
using Adc_A_H_Decoder          = Instantiate<InstructionDecoder<"adc a, h", 0x8C, Adc<RegisterType::A, RegisterType::H>>>::Type;
using Adc_A_L_Decoder          = Instantiate<InstructionDecoder<"adc a, l", 0x8D, Adc<RegisterType::A, RegisterType::L>>>::Type;
using Adc_A_IndirectHL_Decoder = Instantiate<InstructionDecoder<"adc a, [hl]", 0x8E, Adc<RegisterType::A, RegisterType::HL>>>::Type;
using Adc_A_A_Decoder          = Instantiate<InstructionDecoder<"adc a, a", 0x8F, Adc<RegisterType::A, RegisterType::A>>>::Type;

using Sub_A_B_Decoder          = Instantiate<InstructionDecoder<"sub a, b", 0x90, Sub<RegisterType::A, RegisterType::B>>>::Type;
using Sub_A_C_Decoder          = Instantiate<InstructionDecoder<"sub a, c", 0x91, Sub<RegisterType::A, RegisterType::C>>>::Type;
using Sub_A_D_Decoder          = Instantiate<InstructionDecoder<"sub a, d", 0x92, Sub<RegisterType::A, RegisterType::D>>>::Type;
using Sub_A_E_Decoder          = Instantiate<InstructionDecoder<"sub a, e", 0x93, Sub<RegisterType::A, RegisterType::E>>>::Type;
using Sub_A_H_Decoder          = Instantiate<InstructionDecoder<"sub a, h", 0x94, Sub<RegisterType::A, RegisterType::H>>>::Type;
using Sub_A_L_Decoder          = Instantiate<InstructionDecoder<"sub a, l", 0x95, Sub<RegisterType::A, RegisterType::L>>>::Type;
using Sub_A_IndirectHL_Decoder = Instantiate<InstructionDecoder<"sub a, [hl]", 0x96, Sub<RegisterType::A, RegisterType::HL>>>::Type;
using Sub_A_A_Decoder          = Instantiate<InstructionDecoder<"sub a, a", 0x97, Sub<RegisterType::A, RegisterType::A>>>::Type;

using Sbc_A_B_Decoder          = Instantiate<InstructionDecoder<"sbc a, b", 0x98, Sbc<RegisterType::A, RegisterType::B>>>::Type;
using Sbc_A_C_Decoder          = Instantiate<InstructionDecoder<"sbc a, c", 0x99, Sbc<RegisterType::A, RegisterType::C>>>::Type;
using Sbc_A_D_Decoder          = Instantiate<InstructionDecoder<"sbc a, d", 0x9A, Sbc<RegisterType::A, RegisterType::D>>>::Type;
using Sbc_A_E_Decoder          = Instantiate<InstructionDecoder<"sbc a, e", 0x9B, Sbc<RegisterType::A, RegisterType::E>>>::Type;
using Sbc_A_H_Decoder          = Instantiate<InstructionDecoder<"sbc a, h", 0x9C, Sbc<RegisterType::A, RegisterType::H>>>::Type;
using Sbc_A_L_Decoder          = Instantiate<InstructionDecoder<"sbc a, l", 0x9D, Sbc<RegisterType::A, RegisterType::L>>>::Type;
using Sbc_A_IndirectHL_Decoder = Instantiate<InstructionDecoder<"sbc a, [hl]", 0x9E, Sbc<RegisterType::A, RegisterType::HL>>>::Type;
using Sbc_A_A_Decoder          = Instantiate<InstructionDecoder<"sbc a, a", 0x9F, Sbc<RegisterType::A, RegisterType::A>>>::Type;

using And_A_B_Decoder          = Instantiate<InstructionDecoder<"and a, b", 0xA0, And<RegisterType::A, RegisterType::B>>>::Type;
using And_A_C_Decoder          = Instantiate<InstructionDecoder<"and a, c", 0xA1, And<RegisterType::A, RegisterType::C>>>::Type;
using And_A_D_Decoder          = Instantiate<InstructionDecoder<"and a, d", 0xA2, And<RegisterType::A, RegisterType::D>>>::Type;
using And_A_E_Decoder          = Instantiate<InstructionDecoder<"and a, e", 0xA3, And<RegisterType::A, RegisterType::E>>>::Type;
using And_A_H_Decoder          = Instantiate<InstructionDecoder<"and a, h", 0xA4, And<RegisterType::A, RegisterType::H>>>::Type;
using And_A_L_Decoder          = Instantiate<InstructionDecoder<"and a, l", 0xA5, And<RegisterType::A, RegisterType::L>>>::Type;
using And_A_IndirectHL_Decoder = Instantiate<InstructionDecoder<"and a, [hl]", 0xA6, And<RegisterType::A, RegisterType::HL>>>::Type;
using And_A_A_Decoder          = Instantiate<InstructionDecoder<"and a, a", 0xA7, And<RegisterType::A, RegisterType::A>>>::Type;

using Xor_A_B_Decoder          = Instantiate<InstructionDecoder<"xor a, b", 0xA8, Xor<RegisterType::A, RegisterType::B>>>::Type;
using Xor_A_C_Decoder          = Instantiate<InstructionDecoder<"xor a, c", 0xA9, Xor<RegisterType::A, RegisterType::C>>>::Type;
using Xor_A_D_Decoder          = Instantiate<InstructionDecoder<"xor a, d", 0xAA, Xor<RegisterType::A, RegisterType::D>>>::Type;
using Xor_A_E_Decoder          = Instantiate<InstructionDecoder<"xor a, e", 0xAB, Xor<RegisterType::A, RegisterType::E>>>::Type;
using Xor_A_H_Decoder          = Instantiate<InstructionDecoder<"xor a, h", 0xAC, Xor<RegisterType::A, RegisterType::H>>>::Type;
using Xor_A_L_Decoder          = Instantiate<InstructionDecoder<"xor a, l", 0xAD, Xor<RegisterType::A, RegisterType::L>>>::Type;
using Xor_A_IndirectHL_Decoder = Instantiate<InstructionDecoder<"xor a, [hl]", 0xAE, Xor<RegisterType::A, RegisterType::HL>>>::Type;
using Xor_A_A_Decoder          = Instantiate<InstructionDecoder<"xor a, a", 0xAF, Xor<RegisterType::A, RegisterType::A>>>::Type;

using Or_A_B_Decoder           = Instantiate<InstructionDecoder<"or a, b", 0xB0, Or<RegisterType::A, RegisterType::B>>>::Type;
using Or_A_C_Decoder           = Instantiate<InstructionDecoder<"or a, c", 0xB1, Or<RegisterType::A, RegisterType::C>>>::Type;
using Or_A_D_Decoder           = Instantiate<InstructionDecoder<"or a, d", 0xB2, Or<RegisterType::A, RegisterType::D>>>::Type;
using Or_A_E_Decoder           = Instantiate<InstructionDecoder<"or a, e", 0xB3, Or<RegisterType::A, RegisterType::E>>>::Type;
using Or_A_H_Decoder           = Instantiate<InstructionDecoder<"or a, h", 0xB4, Or<RegisterType::A, RegisterType::H>>>::Type;
using Or_A_L_Decoder           = Instantiate<InstructionDecoder<"or a, l", 0xB5, Or<RegisterType::A, RegisterType::L>>>::Type;
using Or_A_IndirectHL_Decoder  = Instantiate<InstructionDecoder<"or a, [hl]", 0xB6, Or<RegisterType::A, RegisterType::HL>>>::Type;
using Or_A_A_Decoder           = Instantiate<InstructionDecoder<"or a, a", 0xB7, Or<RegisterType::A, RegisterType::A>>>::Type;

using Cp_A_B_Decoder           = Instantiate<InstructionDecoder<"cp a, b", 0xB8, Cp<RegisterType::A, RegisterType::B>>>::Type;
using Cp_A_C_Decoder           = Instantiate<InstructionDecoder<"cp a, c", 0xB9, Cp<RegisterType::A, RegisterType::C>>>::Type;
using Cp_A_D_Decoder           = Instantiate<InstructionDecoder<"cp a, d", 0xBA, Cp<RegisterType::A, RegisterType::D>>>::Type;
using Cp_A_E_Decoder           = Instantiate<InstructionDecoder<"cp a, e", 0xBB, Cp<RegisterType::A, RegisterType::E>>>::Type;
using Cp_A_H_Decoder           = Instantiate<InstructionDecoder<"cp a, h", 0xBC, Cp<RegisterType::A, RegisterType::H>>>::Type;
using Cp_A_L_Decoder           = Instantiate<InstructionDecoder<"cp a, l", 0xBD, Cp<RegisterType::A, RegisterType::L>>>::Type;
using Cp_A_IndirectHL_Decoder  = Instantiate<InstructionDecoder<"cp a, [hl]", 0xBE, Cp<RegisterType::A, RegisterType::HL>>>::Type;
using Cp_A_A_Decoder           = Instantiate<InstructionDecoder<"cp a, a", 0xBF, Cp<RegisterType::A, RegisterType::A>>>::Type;

} // namespace pgb::cpu::instruction
