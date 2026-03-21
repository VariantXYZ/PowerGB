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

template <RegisterType Destination, auto Operand, auto Operation, auto Flag>
    requires(IsRegister8Bit<Destination> && (IsRegister8Bit<Operand> || IsRegister16Bit<Operand> || std::is_same_v<decltype(Operand), Byte> || IsRegisterTemp<Operand>))
inline BasicAluResultSet BasicAluOperation(MemoryMap& mmap) noexcept
{
    auto dst = mmap.ReadByte(Destination);
    if (dst.IsFailure())
    {
        return dst;
    }

    Byte srcValue;
    if constexpr (std::is_same_v<decltype(Operand), Byte>)
    {
        srcValue = Operand;
    }
    else if constexpr (IsRegister8Bit<Operand>)
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
    else if constexpr (IsRegisterTemp<Operand>)
    {
        srcValue = Operand == TempHi ? mmap.GetTempHi() : mmap.GetTempLo();
    }
    else
    {
        static_assert(false);
    }

    // Note that src value is specifically not a reference
    auto&              dstValue    = static_cast<const Byte&>(dst);
    std::uint_fast32_t value       = Operation(dstValue, srcValue, mmap.ReadFlag());
    auto               valueCasted = static_cast<Byte>(value);
    auto               result      = mmap.WriteByte(Destination, static_cast<Byte>(valueCasted));

    mmap.WriteFlag(Flag(value, dstValue, srcValue, mmap.ReadFlag()));

    return result;
}

template <RegisterType Destination, auto Operand, bool Carry, bool SetZero = true>
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
                             [](std::int_fast32_t value, Byte dstValue, Byte srcValue, Nibble flag)
                             {
                                 auto valueCasted = static_cast<Byte>(value);
                                 // Flags
                                 bool Z           = SetZero ? valueCasted == 0 : ((flag & 0b1000) != 0);
                                 bool N           = false;
                                 bool H           = ((dstValue ^ srcValue ^ valueCasted) & 0x10) != 0;
                                 bool C           = value > 0xFF;
                                 return Z << 3 | N << 2 | H << 1 | C << 0;
                             }>(mmap);
}

template <RegisterType Destination, auto Operand, bool Carry>
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
                             [](std::int_fast32_t value, Byte dstValue, Byte srcValue, Nibble /*unused*/)
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

template <RegisterType Destination, IncrementMode Mode>
    requires(IsRegister8Bit<Destination>)
inline BasicAluResultSet SingleStepRegisterWithFlag(MemoryMap& mmap) noexcept
{
    static_assert(Mode != IncrementMode::None);
    auto flag   = mmap.ReadFlag();
    auto result = Mode == IncrementMode::Increment ? AddReg<Destination, (Byte)1, false>(mmap) : SubReg<Destination, (Byte)1, false>(mmap);

    // 'C' flag is not affected by 8-bit single step instructions
    mmap.WriteFlag((mmap.ReadFlag() & 0b1110) | (flag & 0b0001));
    return result;
}

template <IncrementMode Mode>
inline BasicAluResultSet SingleStepTempLoWithFlag(MemoryMap& mmap) noexcept
{
    static_assert(Mode != IncrementMode::None);

    const Byte operand = Mode == IncrementMode::Increment ? 1 : -1;

    auto& z            = mmap.GetTempLo();
    Byte  z_           = z + operand;

    bool Z             = z_ == 0;
    bool N             = Mode == IncrementMode::Decrement;
    bool H             = ((z ^ 1 ^ z_) & 0x10) != 0;
    z                  = z_;

    Nibble flag        = Z << 3 | N << 2 | H << 1;
    // Preserve carry flag
    mmap.WriteFlag((mmap.ReadFlag() & 0b0001) | (flag & 0b1110));

    return BasicAluResultSet::DefaultResultSuccess();
}

template <RegisterType Destination, RegisterType Operand>
inline BasicAluResultSet CpReg(MemoryMap& mmap) noexcept
{
    // Sub but without any non-flag writes
    return BasicAluOperation<Destination, Operand, [](Byte dstValue, Byte /*unused*/, Nibble /*unused*/)
                             { return dstValue; },
                             [](std::int_fast32_t /*unused*/, Byte dstValue, Byte srcValue, Nibble /*unused*/)
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
                             [](std::int_fast32_t value, Byte /*unused*/, Byte /*unused*/, Nibble /*unused*/)
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
                             [](std::int_fast32_t value, Byte /*unused*/, Byte /*unused*/, Nibble /*unused*/)
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
                             [](std::int_fast32_t value, Byte /*unused*/, Byte /*unused*/, Nibble /*unused*/)
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

template <RegisterType Destination, bool Circular, bool Zero>
inline BasicAluResultSet RotateLeft(MemoryMap& mmap) noexcept
{
    auto result = mmap.ReadByte(Destination);
    if (result.IsFailure())
    {
        return result;
    }

    auto& d    = static_cast<const Byte&>(result);
    auto  flag = mmap.ReadFlag();

    // C is b7 in both the circular and non-circular case
    // In non-cicular, oldC becomes bit 0
    bool oldC  = flag & 0b0001;
    bool C     = (d & 0b10000000) != 0;
    // Other flags are just set to 0 here
    flag       = C;

    // If Zero is not determined, then we just set it to 0 (for RLCA, RLA)
    // Z if d is 0
    bool Z     = d == 0;
    if constexpr (Zero)
    {

        flag = flag | (Z << 3);
    }

    mmap.WriteFlag(flag);

    if constexpr (Circular)
    {
        return mmap.WriteByte(Destination, static_cast<Byte>((d << 1) | C));
    }
    else
    {
        return mmap.WriteByte(Destination, static_cast<Byte>((d << 1) | oldC));
    }
}

template <RegisterType Destination, bool Circular, bool Zero>
inline BasicAluResultSet RotateRight(MemoryMap& mmap) noexcept
{
    auto result = mmap.ReadByte(Destination);
    if (result.IsFailure())
    {
        return result;
    }

    auto& d    = static_cast<const Byte&>(result);
    auto  flag = mmap.ReadFlag();

    // C is b7 in both the circular and non-circular case
    // In non-cicular, oldC becomes bit 0
    bool oldC  = flag & 0b0001;
    bool C     = (d & 0b00000001) != 0;
    // Other flags are just set to 0 here
    flag       = C;

    // If Zero is not determined, then we just set it to 0 (for RLCA, RLA)
    // Z if d is 0
    bool Z     = d == 0;
    if constexpr (Zero)
    {

        flag = flag | (Z << 3);
    }

    mmap.WriteFlag(flag);

    if constexpr (Circular)
    {
        return mmap.WriteByte(Destination, static_cast<Byte>((d >> 1) | C << 7));
    }
    else
    {
        return mmap.WriteByte(Destination, static_cast<Byte>((d >> 1) | oldC << 7));
    }
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

template <RegisterType Destination, RegisterType Operand>
    requires(IsRegister16Bit<Destination> && IsRegister16Bit<Operand>)
using Add16 = Instruction<
    8,
    AddReg<GetRegisterComponent<Destination, false>(), GetRegisterComponent<Operand, false>(), false, false>,
    AddReg<GetRegisterComponent<Destination, true>(), GetRegisterComponent<Operand, true>(), true, false>,
    IncrementPC,
    LoadIRPC>;

// SP doesn't have 8-bit components, so we just setup a special case for it
template <RegisterType Destination>
    requires(IsRegister16Bit<Destination>)
using AddSp = Instruction<
    8,
    LoadTempReg16<RegisterType::SP>,
    AddReg<GetRegisterComponent<Destination, false>(), RegisterType::TempLo, false, false>,
    AddReg<GetRegisterComponent<Destination, true>(), RegisterType::TempHi, true, false>,
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

// 16-bit increment and decrement, do not set any flags
template <auto Destination, IncrementMode Mode>
    requires(IsRegister16Bit<Destination> && Destination != RegisterType::AF)
using SingleStep16 = Instruction<
    /*Ticks*/ 8,
    SingleStepRegister<Destination, Mode>,
    IncrementPC,
    LoadIRPC>;

template <auto Destination, IncrementMode Mode>
    requires(IsRegister8Bit<Destination> && Destination != RegisterType::F)
using SingleStep8 = Instruction<
    /*Ticks*/ 4,
    SingleStepRegisterWithFlag<Destination, Mode>,
    IncrementPC,
    LoadIRPC>;

template <IncrementMode Mode>
using SingleStepIndirect = Instruction<
    /*Ticks=*/12,
    LoadIndirectReg8TempLo<RegisterType::HL>, // [HL] -> Z
    SingleStepTempLoWithFlag<Mode>,
    LoadReg8TempLoIndirect<RegisterType::HL>, // Z -> [HL]
    IncrementPC,
    LoadIRPC>;

using Inc_BC_Decoder           = Instantiate<InstructionDecoder<"inc bc", 0x03, SingleStep16<RegisterType::BC, IncrementMode::Increment>>>::Type;
using Inc_B_Decoder            = Instantiate<InstructionDecoder<"inc b", 0x04, SingleStep8<RegisterType::B, IncrementMode::Increment>>>::Type;
using Dec_B_Decoder            = Instantiate<InstructionDecoder<"dec b", 0x05, SingleStep8<RegisterType::B, IncrementMode::Decrement>>>::Type;
using Add_HL_BC_Decoder        = Instantiate<InstructionDecoder<"add hl, bc", 0x09, Add16<RegisterType::HL, RegisterType::BC>>>::Type;
using Dec_BC_Decoder           = Instantiate<InstructionDecoder<"dec bc", 0x0B, SingleStep16<RegisterType::BC, IncrementMode::Decrement>>>::Type;
using Inc_C_Decoder            = Instantiate<InstructionDecoder<"inc c", 0x0C, SingleStep8<RegisterType::C, IncrementMode::Increment>>>::Type;
using Dec_C_Decoder            = Instantiate<InstructionDecoder<"dec c", 0x0D, SingleStep8<RegisterType::C, IncrementMode::Decrement>>>::Type;
using Inc_DE_Decoder           = Instantiate<InstructionDecoder<"inc de", 0x13, SingleStep16<RegisterType::DE, IncrementMode::Increment>>>::Type;
using Inc_D_Decoder            = Instantiate<InstructionDecoder<"inc d", 0x14, SingleStep8<RegisterType::D, IncrementMode::Increment>>>::Type;
using Dec_D_Decoder            = Instantiate<InstructionDecoder<"dec d", 0x15, SingleStep8<RegisterType::D, IncrementMode::Decrement>>>::Type;
using Add_HL_DE_Decoder        = Instantiate<InstructionDecoder<"add hl, de", 0x19, Add16<RegisterType::HL, RegisterType::DE>>>::Type;
using Dec_DE_Decoder           = Instantiate<InstructionDecoder<"dec de", 0x1B, SingleStep16<RegisterType::DE, IncrementMode::Decrement>>>::Type;
using Inc_E_Decoder            = Instantiate<InstructionDecoder<"inc e", 0x1C, SingleStep8<RegisterType::E, IncrementMode::Increment>>>::Type;
using Dec_E_Decoder            = Instantiate<InstructionDecoder<"dec e", 0x1D, SingleStep8<RegisterType::E, IncrementMode::Decrement>>>::Type;
using Inc_HL_Decoder           = Instantiate<InstructionDecoder<"inc hl", 0x23, SingleStep16<RegisterType::HL, IncrementMode::Increment>>>::Type;
using Inc_H_Decoder            = Instantiate<InstructionDecoder<"inc h", 0x24, SingleStep8<RegisterType::H, IncrementMode::Increment>>>::Type;
using Dec_H_Decoder            = Instantiate<InstructionDecoder<"dec h", 0x25, SingleStep8<RegisterType::H, IncrementMode::Decrement>>>::Type;
using Dec_HL_Decoder           = Instantiate<InstructionDecoder<"dec hl", 0x2B, SingleStep16<RegisterType::HL, IncrementMode::Decrement>>>::Type;
using Inc_L_Decoder            = Instantiate<InstructionDecoder<"inc l", 0x2C, SingleStep8<RegisterType::L, IncrementMode::Increment>>>::Type;
using Dec_L_Decoder            = Instantiate<InstructionDecoder<"dec l", 0x2D, SingleStep8<RegisterType::L, IncrementMode::Decrement>>>::Type;
using Add_HL_HL_Decoder        = Instantiate<InstructionDecoder<"add hl, hl", 0x29, Add16<RegisterType::HL, RegisterType::HL>>>::Type;
using Inc_SP_Decoder           = Instantiate<InstructionDecoder<"inc sp", 0x33, SingleStep16<RegisterType::SP, IncrementMode::Increment>>>::Type;
using Inc_IndirectHL_Decoder   = Instantiate<InstructionDecoder<"inc [hl]", 0x34, SingleStepIndirect<IncrementMode::Increment>>>::Type;
using Dec_IndirectHL_Decoder   = Instantiate<InstructionDecoder<"dec [hl]", 0x35, SingleStepIndirect<IncrementMode::Decrement>>>::Type;
using Add_HL_SP_Decoder        = Instantiate<InstructionDecoder<"add hl, sp", 0x39, AddSp<RegisterType::HL>>>::Type;
using Dec_SP_Decoder           = Instantiate<InstructionDecoder<"dec sp", 0x3B, SingleStep16<RegisterType::SP, IncrementMode::Decrement>>>::Type;
using Inc_A_Decoder            = Instantiate<InstructionDecoder<"inc a", 0x3C, SingleStep8<RegisterType::A, IncrementMode::Increment>>>::Type;
using Dec_A_Decoder            = Instantiate<InstructionDecoder<"dec a", 0x3D, SingleStep8<RegisterType::A, IncrementMode::Decrement>>>::Type;

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
