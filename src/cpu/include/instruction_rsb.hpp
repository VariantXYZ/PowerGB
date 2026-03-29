#pragma once

#include <cpu/include/decoder.hpp>
#include <cpu/include/instruction.hpp>
#include <cpu/include/registers.hpp>

using namespace pgb::memory;

namespace pgb::cpu::instruction
{

template <typename T, ResultType... Results>
using RsbResultSet =
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

using BasicRsbResultSet = RsbResultSet<void, memory::MemoryMap::ResultRegisterOverflow>;

namespace
{

enum ShiftMode
{
    ShiftArithmetic,
    ShiftLogical,
    Rotate,
    RotateCircular
};

template <RegisterType Destination, ShiftMode Mode, bool Zero>
struct ShiftLeft
{
    static inline BasicRsbResultSet Execute(MemoryMap& mmap) noexcept
        requires IsRegister8Bit<Destination>
    {
        constexpr bool Circular = Mode == ShiftMode::RotateCircular;
        constexpr bool IsRotate = Mode == ShiftMode::Rotate || Mode == ShiftMode::RotateCircular;

        auto result             = mmap.ReadByte(Destination);
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
        // Z if the new value will be 0
        bool Z     = ((d & 0b01111111) == 0) && (IsRotate ? (Circular ? C != 0b1 : oldC != 0b1) : true);
        if constexpr (Zero)
        {
            flag = flag | (Z << 3);
        }

        mmap.WriteFlag(flag);

        if constexpr (IsRotate)
        {
            if constexpr (Circular)
            {
                return mmap.WriteByte(Destination, static_cast<Byte>((d << 1) | C));
            }
            else
            {
                return mmap.WriteByte(Destination, static_cast<Byte>((d << 1) | oldC));
            }
        }
        else
        {
            return mmap.WriteByte(Destination, static_cast<Byte>((d << 1)));
        }
    }

    static inline BasicRsbResultSet Execute(MemoryMap& mmap) noexcept
        requires IsRegister16Bit<Destination>
    {
        constexpr bool Circular = Mode == ShiftMode::RotateCircular;
        constexpr bool IsRotate = Mode == ShiftMode::Rotate || Mode == ShiftMode::RotateCircular;

        auto addrResult         = mmap.ReadWord(Destination);
        if (addrResult.IsFailure())
        {
            return addrResult;
        }
        auto addr   = static_cast<const Word&>(addrResult);

        auto result = mmap.ReadByte(addr);
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
        // Z if the new value will be 0
        bool Z     = ((d & 0b01111111) == 0) && (IsRotate ? (Circular ? C != 0b1 : oldC != 0b1) : true);
        if constexpr (Zero)
        {
            flag = flag | (Z << 3);
        }

        mmap.WriteFlag(flag);

        if constexpr (IsRotate)
        {
            if constexpr (Circular)
            {
                return mmap.WriteByte(addr, static_cast<Byte>((d << 1) | C));
            }
            else
            {
                return mmap.WriteByte(addr, static_cast<Byte>((d << 1) | oldC));
            }
        }
        else
        {
            return mmap.WriteByte(addr, static_cast<Byte>((d << 1)));
        }
    }
};

template <RegisterType Destination, ShiftMode Mode, bool Zero>
struct ShiftRight
{
    static inline BasicRsbResultSet Execute(MemoryMap& mmap) noexcept
        requires IsRegister8Bit<Destination>
    {
        constexpr bool Circular     = Mode == ShiftMode::RotateCircular;
        constexpr bool IsRotate     = Mode == ShiftMode::Rotate || Mode == ShiftMode::RotateCircular;
        constexpr bool IsArithmetic = Mode == ShiftMode::ShiftArithmetic;

        auto result                 = mmap.ReadByte(Destination);
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
        // Z if the new value will be 0
        bool Z     = ((d & 0b11111110) == 0) && (IsRotate ? (Circular ? C != 0b1 : oldC != 0b1) : true);
        if constexpr (Zero)
        {

            flag = flag | (Z << 3);
        }

        mmap.WriteFlag(flag);

        if constexpr (IsRotate)
        {
            if constexpr (Circular)
            {
                return mmap.WriteByte(Destination, static_cast<Byte>((d >> 1) | C << 7));
            }
            else
            {
                return mmap.WriteByte(Destination, static_cast<Byte>((d >> 1) | oldC << 7));
            }
        }
        else
        {
            if constexpr (IsArithmetic)
            {
                return mmap.WriteByte(Destination, static_cast<Byte>((d >> 1) | (d & 0b10000000)));
            }
            else
            {
                return mmap.WriteByte(Destination, static_cast<Byte>((d >> 1)));
            }
        }
    }

    static inline BasicRsbResultSet Execute(MemoryMap& mmap) noexcept
        requires IsRegister16Bit<Destination>
    {
        constexpr bool Circular     = Mode == ShiftMode::RotateCircular;
        constexpr bool IsRotate     = Mode == ShiftMode::Rotate || Mode == ShiftMode::RotateCircular;
        constexpr bool IsArithmetic = Mode == ShiftMode::ShiftArithmetic;

        auto addrResult             = mmap.ReadWord(Destination);
        if (addrResult.IsFailure())
        {
            return addrResult;
        }

        auto addr   = static_cast<const Word&>(addrResult);

        auto result = mmap.ReadByte(addr);
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
        // Z if the new value will be 0
        bool Z     = ((d & 0b11111110) == 0) && (IsRotate ? (Circular ? C != 0b1 : oldC != 0b1) : true);
        if constexpr (Zero)
        {
            flag = flag | (Z << 3);
        }

        mmap.WriteFlag(flag);

        if constexpr (IsRotate)
        {
            if constexpr (Circular)
            {
                return mmap.WriteByte(addr, static_cast<Byte>((d >> 1) | C << 7));
            }
            else
            {
                return mmap.WriteByte(addr, static_cast<Byte>((d >> 1) | oldC << 7));
            }
        }
        else
        {
            if constexpr (IsArithmetic)
            {
                return mmap.WriteByte(addr, static_cast<Byte>((d >> 1) | (d & 0b10000000)));
            }
            else
            {
                return mmap.WriteByte(addr, static_cast<Byte>((d >> 1)));
            }
        }
    }
};

template <RegisterType Destination>
struct SwapNibble
{
    static inline BasicRsbResultSet Execute(MemoryMap& mmap) noexcept
    {
        std::uint_fast16_t addr;
        if constexpr (IsRegister16Bit<Destination>)
        {
            auto addrResult = mmap.ReadWord(Destination);
            if (addrResult.IsFailure())
            {
                return addrResult;
            }

            addr = static_cast<const Word&>(addrResult);
        }

        auto result = IsRegister16Bit<Destination> ? static_cast<RsbResultSet<const Byte&>>(mmap.ReadByte(addr)) : static_cast<RsbResultSet<const Byte&>>(mmap.ReadByte(Destination));
        if (result.IsFailure())
        {
            return result;
        }

        auto& d = static_cast<const Byte&>(result);
        bool  Z = d == 0;
        mmap.WriteFlag(Z << 3);

        if constexpr (IsRegister16Bit<Destination>)
        {
            return mmap.WriteByte(addr, Byte(d.LowNibble(), d.HighNibble()));
        }
        else
        {
            return mmap.WriteByte(Destination, Byte(d.LowNibble(), d.HighNibble()));
        }
    }
};

template <RegisterType Register, std::size_t N>
    requires(N >= 0 && N <= 8)
struct TestBit
{
    static inline BasicRsbResultSet Execute(MemoryMap& mmap) noexcept
    {
        std::uint_fast16_t addr;
        if constexpr (IsRegister16Bit<Register>)
        {
            auto addrResult = mmap.ReadWord(Register);
            if (addrResult.IsFailure())
            {
                return addrResult;
            }

            addr = static_cast<const Word&>(addrResult);
        }

        auto result = IsRegister16Bit<Register> ? static_cast<RsbResultSet<const Byte&>>(mmap.ReadByte(addr)) : static_cast<RsbResultSet<const Byte&>>(mmap.ReadByte(Register));

        if (result.IsFailure())
        {
            return result;
        }

        auto& d = static_cast<const Byte&>(result);
        bool  Z = ((d >> N) & 0b1) == 0;

        mmap.WriteFlagBit(memory::MemoryMap::Zero, Z);
        mmap.WriteFlagBit(memory::MemoryMap::Subtract, false);
        mmap.WriteFlagBit(memory::MemoryMap::HalfCarry, true);

        return BasicRsbResultSet::DefaultResultSuccess();
    }
};

template <RegisterType Destination, bool Circular, bool Zero>
using Rl = Instruction<
    /*Ticks*/ IsRegister8Bit<Destination> ? 4 : 12,
    ShiftLeft<Destination, Circular ? ShiftMode::RotateCircular : ShiftMode::Rotate, Zero>,
    IncrementPC,
    LoadIRPC>;

template <RegisterType Destination, bool Circular, bool Zero>
using Rr = Instruction<
    /*Ticks*/ IsRegister8Bit<Destination> ? 4 : 12,
    ShiftRight<Destination, Circular ? ShiftMode::RotateCircular : ShiftMode::Rotate, Zero>,
    IncrementPC,
    LoadIRPC>;

template <RegisterType Destination>
using Sla = Instruction<
    /*Ticks*/ IsRegister8Bit<Destination> ? 4 : 12,
    ShiftLeft<Destination, ShiftMode::ShiftArithmetic, true>,
    IncrementPC,
    LoadIRPC>;

template <RegisterType Destination>
using Sra = Instruction<
    /*Ticks*/ IsRegister8Bit<Destination> ? 4 : 12,
    ShiftRight<Destination, ShiftMode::ShiftArithmetic, true>,
    IncrementPC,
    LoadIRPC>;

template <RegisterType Destination>
using Srl = Instruction<
    /*Ticks*/ IsRegister8Bit<Destination> ? 4 : 12,
    ShiftRight<Destination, ShiftMode::ShiftLogical, true>,
    IncrementPC,
    LoadIRPC>;

template <RegisterType Destination>
using Swap = Instruction<
    /*Ticks*/ IsRegister8Bit<Destination> ? 4 : 12,
    SwapNibble<Destination>,
    IncrementPC,
    LoadIRPC>;

template <RegisterType Register, std::size_t N>
using Bit = Instruction<
    /*Ticks*/ IsRegister8Bit<Register> ? 4 : 8,
    TestBit<Register, N>,
    IncrementPC,
    LoadIRPC>;

} // namespace

using RLCA_Decoder            = Instantiate<InstructionDecoder<"rlca", 0x07, Rl<RegisterType::A, true, false>>>::Type;
using RRCA_Decoder            = Instantiate<InstructionDecoder<"rrca", 0x0F, Rr<RegisterType::A, true, false>>>::Type;
using RLA_Decoder             = Instantiate<InstructionDecoder<"rla", 0x17, Rl<RegisterType::A, false, false>>>::Type;
using RRA_Decoder             = Instantiate<InstructionDecoder<"rra", 0x1F, Rr<RegisterType::A, false, false>>>::Type;

// CB Prefix
using RLC_B_Decoder           = Instantiate<InstructionDecoder<"rlc b", 0x00, Rl<RegisterType::B, true, true>, 0xCB>>::Type;
using RLC_C_Decoder           = Instantiate<InstructionDecoder<"rlc c", 0x01, Rl<RegisterType::C, true, true>, 0xCB>>::Type;
using RLC_D_Decoder           = Instantiate<InstructionDecoder<"rlc d", 0x02, Rl<RegisterType::D, true, true>, 0xCB>>::Type;
using RLC_E_Decoder           = Instantiate<InstructionDecoder<"rlc e", 0x03, Rl<RegisterType::E, true, true>, 0xCB>>::Type;
using RLC_H_Decoder           = Instantiate<InstructionDecoder<"rlc h", 0x04, Rl<RegisterType::H, true, true>, 0xCB>>::Type;
using RLC_L_Decoder           = Instantiate<InstructionDecoder<"rlc l", 0x05, Rl<RegisterType::L, true, true>, 0xCB>>::Type;
using RLC_IndirectHL_Decoder  = Instantiate<InstructionDecoder<"rlc [hl]", 0x06, Rl<RegisterType::HL, true, true>, 0xCB>>::Type;
using RLC_A_Decoder           = Instantiate<InstructionDecoder<"rlc a", 0x07, Rl<RegisterType::A, true, true>, 0xCB>>::Type;
using RRC_B_Decoder           = Instantiate<InstructionDecoder<"rrc b", 0x08, Rr<RegisterType::B, true, true>, 0xCB>>::Type;
using RRC_C_Decoder           = Instantiate<InstructionDecoder<"rrc c", 0x09, Rr<RegisterType::C, true, true>, 0xCB>>::Type;
using RRC_D_Decoder           = Instantiate<InstructionDecoder<"rrc d", 0x0A, Rr<RegisterType::D, true, true>, 0xCB>>::Type;
using RRC_E_Decoder           = Instantiate<InstructionDecoder<"rrc e", 0x0B, Rr<RegisterType::E, true, true>, 0xCB>>::Type;
using RRC_H_Decoder           = Instantiate<InstructionDecoder<"rrc h", 0x0C, Rr<RegisterType::H, true, true>, 0xCB>>::Type;
using RRC_L_Decoder           = Instantiate<InstructionDecoder<"rrc l", 0x0D, Rr<RegisterType::L, true, true>, 0xCB>>::Type;
using RRC_IndirectHL_Decoder  = Instantiate<InstructionDecoder<"rrc [hl]", 0x0E, Rr<RegisterType::HL, true, true>, 0xCB>>::Type;
using RRC_A_Decoder           = Instantiate<InstructionDecoder<"rrc a", 0x0F, Rr<RegisterType::A, true, true>, 0xCB>>::Type;
using RL_B_Decoder            = Instantiate<InstructionDecoder<"rl b", 0x10, Rl<RegisterType::B, false, true>, 0xCB>>::Type;
using RL_C_Decoder            = Instantiate<InstructionDecoder<"rl c", 0x11, Rl<RegisterType::C, false, true>, 0xCB>>::Type;
using RL_D_Decoder            = Instantiate<InstructionDecoder<"rl d", 0x12, Rl<RegisterType::D, false, true>, 0xCB>>::Type;
using RL_E_Decoder            = Instantiate<InstructionDecoder<"rl e", 0x13, Rl<RegisterType::E, false, true>, 0xCB>>::Type;
using RL_H_Decoder            = Instantiate<InstructionDecoder<"rl h", 0x14, Rl<RegisterType::H, false, true>, 0xCB>>::Type;
using RL_L_Decoder            = Instantiate<InstructionDecoder<"rl l", 0x15, Rl<RegisterType::L, false, true>, 0xCB>>::Type;
using RL_IndirectHL_Decoder   = Instantiate<InstructionDecoder<"rl [hl]", 0x16, Rl<RegisterType::HL, false, true>, 0xCB>>::Type;
using RL_A_Decoder            = Instantiate<InstructionDecoder<"rl a", 0x17, Rl<RegisterType::A, false, true>, 0xCB>>::Type;
using RR_B_Decoder            = Instantiate<InstructionDecoder<"rr b", 0x18, Rr<RegisterType::B, false, true>, 0xCB>>::Type;
using RR_C_Decoder            = Instantiate<InstructionDecoder<"rr c", 0x19, Rr<RegisterType::C, false, true>, 0xCB>>::Type;
using RR_D_Decoder            = Instantiate<InstructionDecoder<"rr d", 0x1A, Rr<RegisterType::D, false, true>, 0xCB>>::Type;
using RR_E_Decoder            = Instantiate<InstructionDecoder<"rr e", 0x1B, Rr<RegisterType::E, false, true>, 0xCB>>::Type;
using RR_H_Decoder            = Instantiate<InstructionDecoder<"rr h", 0x1C, Rr<RegisterType::H, false, true>, 0xCB>>::Type;
using RR_L_Decoder            = Instantiate<InstructionDecoder<"rr l", 0x1D, Rr<RegisterType::L, false, true>, 0xCB>>::Type;
using RR_IndirectHL_Decoder   = Instantiate<InstructionDecoder<"rr [hl]", 0x1E, Rr<RegisterType::HL, false, true>, 0xCB>>::Type;
using RR_A_Decoder            = Instantiate<InstructionDecoder<"rr a", 0x1F, Rr<RegisterType::A, false, true>, 0xCB>>::Type;
using SLA_B_Decoder           = Instantiate<InstructionDecoder<"sla b", 0x20, Sla<RegisterType::B>, 0xCB>>::Type;
using SLA_C_Decoder           = Instantiate<InstructionDecoder<"sla c", 0x21, Sla<RegisterType::C>, 0xCB>>::Type;
using SLA_D_Decoder           = Instantiate<InstructionDecoder<"sla d", 0x22, Sla<RegisterType::D>, 0xCB>>::Type;
using SLA_E_Decoder           = Instantiate<InstructionDecoder<"sla e", 0x23, Sla<RegisterType::E>, 0xCB>>::Type;
using SLA_H_Decoder           = Instantiate<InstructionDecoder<"sla h", 0x24, Sla<RegisterType::H>, 0xCB>>::Type;
using SLA_L_Decoder           = Instantiate<InstructionDecoder<"sla l", 0x25, Sla<RegisterType::L>, 0xCB>>::Type;
using SLA_IndirectHL_Decoder  = Instantiate<InstructionDecoder<"sla [hl]", 0x26, Sla<RegisterType::HL>, 0xCB>>::Type;
using SLA_A_Decoder           = Instantiate<InstructionDecoder<"sla a", 0x27, Sla<RegisterType::A>, 0xCB>>::Type;
using SRA_B_Decoder           = Instantiate<InstructionDecoder<"sra b", 0x28, Sra<RegisterType::B>, 0xCB>>::Type;
using SRA_C_Decoder           = Instantiate<InstructionDecoder<"sra c", 0x29, Sra<RegisterType::C>, 0xCB>>::Type;
using SRA_D_Decoder           = Instantiate<InstructionDecoder<"sra d", 0x2A, Sra<RegisterType::D>, 0xCB>>::Type;
using SRA_E_Decoder           = Instantiate<InstructionDecoder<"sra e", 0x2B, Sra<RegisterType::E>, 0xCB>>::Type;
using SRA_H_Decoder           = Instantiate<InstructionDecoder<"sra h", 0x2C, Sra<RegisterType::H>, 0xCB>>::Type;
using SRA_L_Decoder           = Instantiate<InstructionDecoder<"sra l", 0x2D, Sra<RegisterType::L>, 0xCB>>::Type;
using SRA_IndirectHL_Decoder  = Instantiate<InstructionDecoder<"sra [hl]", 0x2E, Sra<RegisterType::HL>, 0xCB>>::Type;
using SRA_A_Decoder           = Instantiate<InstructionDecoder<"sra a", 0x2F, Sra<RegisterType::A>, 0xCB>>::Type;
using SWAP_B_Decoder          = Instantiate<InstructionDecoder<"swap b", 0x30, Swap<RegisterType::B>, 0xCB>>::Type;
using SWAP_C_Decoder          = Instantiate<InstructionDecoder<"swap c", 0x31, Swap<RegisterType::C>, 0xCB>>::Type;
using SWAP_D_Decoder          = Instantiate<InstructionDecoder<"swap d", 0x32, Swap<RegisterType::D>, 0xCB>>::Type;
using SWAP_E_Decoder          = Instantiate<InstructionDecoder<"swap e", 0x33, Swap<RegisterType::E>, 0xCB>>::Type;
using SWAP_H_Decoder          = Instantiate<InstructionDecoder<"swap h", 0x34, Swap<RegisterType::H>, 0xCB>>::Type;
using SWAP_L_Decoder          = Instantiate<InstructionDecoder<"swap l", 0x35, Swap<RegisterType::L>, 0xCB>>::Type;
using SWAP_IndirectHL_Decoder = Instantiate<InstructionDecoder<"swap [hl]", 0x36, Swap<RegisterType::HL>, 0xCB>>::Type;
using SWAP_A_Decoder          = Instantiate<InstructionDecoder<"swap a", 0x37, Swap<RegisterType::A>, 0xCB>>::Type;
using SRL_B_Decoder           = Instantiate<InstructionDecoder<"srl b", 0x38, Srl<RegisterType::B>, 0xCB>>::Type;
using SRL_C_Decoder           = Instantiate<InstructionDecoder<"srl c", 0x39, Srl<RegisterType::C>, 0xCB>>::Type;
using SRL_D_Decoder           = Instantiate<InstructionDecoder<"srl d", 0x3A, Srl<RegisterType::D>, 0xCB>>::Type;
using SRL_E_Decoder           = Instantiate<InstructionDecoder<"srl e", 0x3B, Srl<RegisterType::E>, 0xCB>>::Type;
using SRL_H_Decoder           = Instantiate<InstructionDecoder<"srl h", 0x3C, Srl<RegisterType::H>, 0xCB>>::Type;
using SRL_L_Decoder           = Instantiate<InstructionDecoder<"srl l", 0x3D, Srl<RegisterType::L>, 0xCB>>::Type;
using SRL_IndirectHL_Decoder  = Instantiate<InstructionDecoder<"srl [hl]", 0x3E, Srl<RegisterType::HL>, 0xCB>>::Type;
using SRL_A_Decoder           = Instantiate<InstructionDecoder<"srl a", 0x3F, Srl<RegisterType::A>, 0xCB>>::Type;
using Bit0_B_Decoder          = Instantiate<InstructionDecoder<"bit 0,b", 0x40, Bit<RegisterType::B, 0>, 0xCB>>::Type;
using Bit0_C_Decoder          = Instantiate<InstructionDecoder<"bit 0,c", 0x41, Bit<RegisterType::C, 0>, 0xCB>>::Type;
using Bit0_D_Decoder          = Instantiate<InstructionDecoder<"bit 0,d", 0x42, Bit<RegisterType::D, 0>, 0xCB>>::Type;
using Bit0_E_Decoder          = Instantiate<InstructionDecoder<"bit 0,e", 0x43, Bit<RegisterType::E, 0>, 0xCB>>::Type;
using Bit0_H_Decoder          = Instantiate<InstructionDecoder<"bit 0,h", 0x44, Bit<RegisterType::H, 0>, 0xCB>>::Type;
using Bit0_L_Decoder          = Instantiate<InstructionDecoder<"bit 0,l", 0x45, Bit<RegisterType::L, 0>, 0xCB>>::Type;
using Bit0_IndirectHL_Decoder = Instantiate<InstructionDecoder<"bit 0,[hl]", 0x46, Bit<RegisterType::HL, 0>, 0xCB>>::Type;
using Bit0_A_Decoder          = Instantiate<InstructionDecoder<"bit 0,a", 0x47, Bit<RegisterType::A, 0>, 0xCB>>::Type;
using Bit1_B_Decoder          = Instantiate<InstructionDecoder<"bit 1,b", 0x48, Bit<RegisterType::B, 1>, 0xCB>>::Type;
using Bit1_C_Decoder          = Instantiate<InstructionDecoder<"bit 1,c", 0x49, Bit<RegisterType::C, 1>, 0xCB>>::Type;
using Bit1_D_Decoder          = Instantiate<InstructionDecoder<"bit 1,d", 0x4A, Bit<RegisterType::D, 1>, 0xCB>>::Type;
using Bit1_E_Decoder          = Instantiate<InstructionDecoder<"bit 1,e", 0x4B, Bit<RegisterType::E, 1>, 0xCB>>::Type;
using Bit1_H_Decoder          = Instantiate<InstructionDecoder<"bit 1,h", 0x4C, Bit<RegisterType::H, 1>, 0xCB>>::Type;
using Bit1_L_Decoder          = Instantiate<InstructionDecoder<"bit 1,l", 0x4D, Bit<RegisterType::L, 1>, 0xCB>>::Type;
using Bit1_IndirectHL_Decoder = Instantiate<InstructionDecoder<"bit 1,[hl]", 0x4E, Bit<RegisterType::HL, 1>, 0xCB>>::Type;
using Bit1_A_Decoder          = Instantiate<InstructionDecoder<"bit 1,a", 0x4F, Bit<RegisterType::A, 1>, 0xCB>>::Type;
using Bit2_B_Decoder          = Instantiate<InstructionDecoder<"bit 2,b", 0x50, Bit<RegisterType::B, 2>, 0xCB>>::Type;
using Bit2_C_Decoder          = Instantiate<InstructionDecoder<"bit 2,c", 0x51, Bit<RegisterType::C, 2>, 0xCB>>::Type;
using Bit2_D_Decoder          = Instantiate<InstructionDecoder<"bit 2,d", 0x52, Bit<RegisterType::D, 2>, 0xCB>>::Type;
using Bit2_E_Decoder          = Instantiate<InstructionDecoder<"bit 2,e", 0x53, Bit<RegisterType::E, 2>, 0xCB>>::Type;
using Bit2_H_Decoder          = Instantiate<InstructionDecoder<"bit 2,h", 0x54, Bit<RegisterType::H, 2>, 0xCB>>::Type;
using Bit2_L_Decoder          = Instantiate<InstructionDecoder<"bit 2,l", 0x55, Bit<RegisterType::L, 2>, 0xCB>>::Type;
using Bit2_IndirectHL_Decoder = Instantiate<InstructionDecoder<"bit 2,[hl]", 0x56, Bit<RegisterType::HL, 2>, 0xCB>>::Type;
using Bit2_A_Decoder          = Instantiate<InstructionDecoder<"bit 2,a", 0x57, Bit<RegisterType::A, 2>, 0xCB>>::Type;
using Bit3_B_Decoder          = Instantiate<InstructionDecoder<"bit 3,b", 0x58, Bit<RegisterType::B, 3>, 0xCB>>::Type;
using Bit3_C_Decoder          = Instantiate<InstructionDecoder<"bit 3,c", 0x59, Bit<RegisterType::C, 3>, 0xCB>>::Type;
using Bit3_D_Decoder          = Instantiate<InstructionDecoder<"bit 3,d", 0x5A, Bit<RegisterType::D, 3>, 0xCB>>::Type;
using Bit3_E_Decoder          = Instantiate<InstructionDecoder<"bit 3,e", 0x5B, Bit<RegisterType::E, 3>, 0xCB>>::Type;
using Bit3_H_Decoder          = Instantiate<InstructionDecoder<"bit 3,h", 0x5C, Bit<RegisterType::H, 3>, 0xCB>>::Type;
using Bit3_L_Decoder          = Instantiate<InstructionDecoder<"bit 3,l", 0x5D, Bit<RegisterType::L, 3>, 0xCB>>::Type;
using Bit3_IndirectHL_Decoder = Instantiate<InstructionDecoder<"bit 3,[hl]", 0x5E, Bit<RegisterType::HL, 3>, 0xCB>>::Type;
using Bit3_A_Decoder          = Instantiate<InstructionDecoder<"bit 3,a", 0x5F, Bit<RegisterType::A, 3>, 0xCB>>::Type;
using Bit4_B_Decoder          = Instantiate<InstructionDecoder<"bit 4,b", 0x60, Bit<RegisterType::B, 4>, 0xCB>>::Type;
using Bit4_C_Decoder          = Instantiate<InstructionDecoder<"bit 4,c", 0x61, Bit<RegisterType::C, 4>, 0xCB>>::Type;
using Bit4_D_Decoder          = Instantiate<InstructionDecoder<"bit 4,d", 0x62, Bit<RegisterType::D, 4>, 0xCB>>::Type;
using Bit4_E_Decoder          = Instantiate<InstructionDecoder<"bit 4,e", 0x63, Bit<RegisterType::E, 4>, 0xCB>>::Type;
using Bit4_H_Decoder          = Instantiate<InstructionDecoder<"bit 4,h", 0x64, Bit<RegisterType::H, 4>, 0xCB>>::Type;
using Bit4_L_Decoder          = Instantiate<InstructionDecoder<"bit 4,l", 0x65, Bit<RegisterType::L, 4>, 0xCB>>::Type;
using Bit4_IndirectHL_Decoder = Instantiate<InstructionDecoder<"bit 4,[hl]", 0x66, Bit<RegisterType::HL, 4>, 0xCB>>::Type;
using Bit4_A_Decoder          = Instantiate<InstructionDecoder<"bit 4,a", 0x67, Bit<RegisterType::A, 4>, 0xCB>>::Type;
using Bit5_B_Decoder          = Instantiate<InstructionDecoder<"bit 5,b", 0x68, Bit<RegisterType::B, 5>, 0xCB>>::Type;
using Bit5_C_Decoder          = Instantiate<InstructionDecoder<"bit 5,c", 0x69, Bit<RegisterType::C, 5>, 0xCB>>::Type;
using Bit5_D_Decoder          = Instantiate<InstructionDecoder<"bit 5,d", 0x6A, Bit<RegisterType::D, 5>, 0xCB>>::Type;
using Bit5_E_Decoder          = Instantiate<InstructionDecoder<"bit 5,e", 0x6B, Bit<RegisterType::E, 5>, 0xCB>>::Type;
using Bit5_H_Decoder          = Instantiate<InstructionDecoder<"bit 5,h", 0x6C, Bit<RegisterType::H, 5>, 0xCB>>::Type;
using Bit5_L_Decoder          = Instantiate<InstructionDecoder<"bit 5,l", 0x6D, Bit<RegisterType::L, 5>, 0xCB>>::Type;
using Bit5_IndirectHL_Decoder = Instantiate<InstructionDecoder<"bit 5,[hl]", 0x6E, Bit<RegisterType::HL, 5>, 0xCB>>::Type;
using Bit5_A_Decoder          = Instantiate<InstructionDecoder<"bit 5,a", 0x6F, Bit<RegisterType::A, 5>, 0xCB>>::Type;
using Bit6_B_Decoder          = Instantiate<InstructionDecoder<"bit 6,b", 0x70, Bit<RegisterType::B, 6>, 0xCB>>::Type;
using Bit6_C_Decoder          = Instantiate<InstructionDecoder<"bit 6,c", 0x71, Bit<RegisterType::C, 6>, 0xCB>>::Type;
using Bit6_D_Decoder          = Instantiate<InstructionDecoder<"bit 6,d", 0x72, Bit<RegisterType::D, 6>, 0xCB>>::Type;
using Bit6_E_Decoder          = Instantiate<InstructionDecoder<"bit 6,e", 0x73, Bit<RegisterType::E, 6>, 0xCB>>::Type;
using Bit6_H_Decoder          = Instantiate<InstructionDecoder<"bit 6,h", 0x74, Bit<RegisterType::H, 6>, 0xCB>>::Type;
using Bit6_L_Decoder          = Instantiate<InstructionDecoder<"bit 6,l", 0x75, Bit<RegisterType::L, 6>, 0xCB>>::Type;
using Bit6_IndirectHL_Decoder = Instantiate<InstructionDecoder<"bit 6,[hl]", 0x76, Bit<RegisterType::HL, 6>, 0xCB>>::Type;
using Bit6_A_Decoder          = Instantiate<InstructionDecoder<"bit 6,a", 0x77, Bit<RegisterType::A, 6>, 0xCB>>::Type;
using Bit7_B_Decoder          = Instantiate<InstructionDecoder<"bit 7,b", 0x78, Bit<RegisterType::B, 7>, 0xCB>>::Type;
using Bit7_C_Decoder          = Instantiate<InstructionDecoder<"bit 7,c", 0x79, Bit<RegisterType::C, 7>, 0xCB>>::Type;
using Bit7_D_Decoder          = Instantiate<InstructionDecoder<"bit 7,d", 0x7A, Bit<RegisterType::D, 7>, 0xCB>>::Type;
using Bit7_E_Decoder          = Instantiate<InstructionDecoder<"bit 7,e", 0x7B, Bit<RegisterType::E, 7>, 0xCB>>::Type;
using Bit7_H_Decoder          = Instantiate<InstructionDecoder<"bit 7,h", 0x7C, Bit<RegisterType::H, 7>, 0xCB>>::Type;
using Bit7_L_Decoder          = Instantiate<InstructionDecoder<"bit 7,l", 0x7D, Bit<RegisterType::L, 7>, 0xCB>>::Type;
using Bit7_IndirectHL_Decoder = Instantiate<InstructionDecoder<"bit 7,[hl]", 0x7E, Bit<RegisterType::HL, 7>, 0xCB>>::Type;
using Bit7_A_Decoder          = Instantiate<InstructionDecoder<"bit 7,a", 0x7F, Bit<RegisterType::A, 7>, 0xCB>>::Type;

} // namespace pgb::cpu::instruction
