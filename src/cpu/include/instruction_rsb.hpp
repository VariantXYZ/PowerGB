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

template <RegisterType Destination, bool Circular, bool Zero>
struct RotateLeft
{
    static inline BasicRsbResultSet Execute(MemoryMap& mmap) noexcept
        requires IsRegister8Bit<Destination>
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
        // Z if the new value will be 0
        bool Z     = ((d & 0b01111111) == 0) && (Circular ? C != 0b1 : oldC != 0b1);
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

    static inline BasicRsbResultSet Execute(MemoryMap& mmap) noexcept
        requires IsRegister16Bit<Destination>
    {
        auto addrResult = mmap.ReadWord(Destination);
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
        bool Z     = ((d & 0b01111111) == 0) && (Circular ? C != 0b1 : oldC != 0b1);
        if constexpr (Zero)
        {
            flag = flag | (Z << 3);
        }

        mmap.WriteFlag(flag);

        if constexpr (Circular)
        {
            return mmap.WriteByte(addr, static_cast<Byte>((d << 1) | C));
        }
        else
        {
            return mmap.WriteByte(addr, static_cast<Byte>((d << 1) | oldC));
        }
    }
};

template <RegisterType Destination, bool Circular, bool Zero>
struct RotateRight
{
    static inline BasicRsbResultSet Execute(MemoryMap& mmap) noexcept
        requires IsRegister8Bit<Destination>
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
        // Z if the new value will be 0
        bool Z     = ((d & 0b11111110) == 0) && (Circular ? C != 0b1 : oldC != 0b1);
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

    static inline BasicRsbResultSet Execute(MemoryMap& mmap) noexcept
        requires IsRegister16Bit<Destination>
    {
        auto addrResult = mmap.ReadWord(Destination);
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
        bool Z     = ((d & 0b11111110) == 0) && (Circular ? C != 0b1 : oldC != 0b1);
        if constexpr (Zero)
        {

            flag = flag | (Z << 3);
        }

        mmap.WriteFlag(flag);

        if constexpr (Circular)
        {
            return mmap.WriteByte(addr, static_cast<Byte>((d >> 1) | C << 7));
        }
        else
        {
            return mmap.WriteByte(addr, static_cast<Byte>((d >> 1) | oldC << 7));
        }
    }
};

template <RegisterType Destination, bool Circular, bool Zero>
using Rl = Instruction<
    /*Ticks*/ IsRegister8Bit<Destination> ? 4 : 12,
    RotateLeft<Destination, Circular, Zero>,
    IncrementPC,
    LoadIRPC>;

template <RegisterType Destination, bool Circular, bool Zero>
using Rr = Instruction<
    /*Ticks*/ IsRegister8Bit<Destination> ? 4 : 12,
    RotateRight<Destination, Circular, Zero>,
    IncrementPC,
    LoadIRPC>;

using RLCA_Decoder           = Instantiate<InstructionDecoder<"rlca", 0x07, Rl<RegisterType::A, true, false>>>::Type;
using RRCA_Decoder           = Instantiate<InstructionDecoder<"rrca", 0x0F, Rr<RegisterType::A, true, false>>>::Type;
using RLA_Decoder            = Instantiate<InstructionDecoder<"rla", 0x17, Rl<RegisterType::A, false, false>>>::Type;
using RRA_Decoder            = Instantiate<InstructionDecoder<"rra", 0x1F, Rr<RegisterType::A, false, false>>>::Type;

// CB Prefix
using RLC_B_Decoder          = Instantiate<InstructionDecoder<"rlc b", 0x00, Rl<RegisterType::B, true, true>, 0xCB>>::Type;
using RLC_C_Decoder          = Instantiate<InstructionDecoder<"rlc c", 0x01, Rl<RegisterType::C, true, true>, 0xCB>>::Type;
using RLC_D_Decoder          = Instantiate<InstructionDecoder<"rlc d", 0x02, Rl<RegisterType::D, true, true>, 0xCB>>::Type;
using RLC_E_Decoder          = Instantiate<InstructionDecoder<"rlc e", 0x03, Rl<RegisterType::E, true, true>, 0xCB>>::Type;
using RLC_H_Decoder          = Instantiate<InstructionDecoder<"rlc h", 0x04, Rl<RegisterType::H, true, true>, 0xCB>>::Type;
using RLC_L_Decoder          = Instantiate<InstructionDecoder<"rlc l", 0x05, Rl<RegisterType::L, true, true>, 0xCB>>::Type;
using RLC_IndirectHL_Decoder = Instantiate<InstructionDecoder<"rlc [hl]", 0x06, Rl<RegisterType::HL, true, true>, 0xCB>>::Type;
using RLC_A_Decoder          = Instantiate<InstructionDecoder<"rlc a", 0x07, Rl<RegisterType::A, true, true>, 0xCB>>::Type;
using RRC_B_Decoder          = Instantiate<InstructionDecoder<"rrc b", 0x08, Rr<RegisterType::B, true, true>, 0xCB>>::Type;
using RRC_C_Decoder          = Instantiate<InstructionDecoder<"rrc c", 0x09, Rr<RegisterType::C, true, true>, 0xCB>>::Type;
using RRC_D_Decoder          = Instantiate<InstructionDecoder<"rrc d", 0x0A, Rr<RegisterType::D, true, true>, 0xCB>>::Type;
using RRC_E_Decoder          = Instantiate<InstructionDecoder<"rrc e", 0x0B, Rr<RegisterType::E, true, true>, 0xCB>>::Type;
using RRC_H_Decoder          = Instantiate<InstructionDecoder<"rrc h", 0x0C, Rr<RegisterType::H, true, true>, 0xCB>>::Type;
using RRC_L_Decoder          = Instantiate<InstructionDecoder<"rrc l", 0x0D, Rr<RegisterType::L, true, true>, 0xCB>>::Type;
using RRC_IndirectHL_Decoder = Instantiate<InstructionDecoder<"rrc [hl]", 0x0E, Rr<RegisterType::HL, true, true>, 0xCB>>::Type;
using RRC_A_Decoder          = Instantiate<InstructionDecoder<"rrc a", 0x0F, Rr<RegisterType::A, true, true>, 0xCB>>::Type;

using RL_B_Decoder           = Instantiate<InstructionDecoder<"rl b", 0x10, Rl<RegisterType::B, false, true>, 0xCB>>::Type;
using RL_C_Decoder           = Instantiate<InstructionDecoder<"rl c", 0x11, Rl<RegisterType::C, false, true>, 0xCB>>::Type;
using RL_D_Decoder           = Instantiate<InstructionDecoder<"rl d", 0x12, Rl<RegisterType::D, false, true>, 0xCB>>::Type;
using RL_E_Decoder           = Instantiate<InstructionDecoder<"rl e", 0x13, Rl<RegisterType::E, false, true>, 0xCB>>::Type;
using RL_H_Decoder           = Instantiate<InstructionDecoder<"rl h", 0x14, Rl<RegisterType::H, false, true>, 0xCB>>::Type;
using RL_L_Decoder           = Instantiate<InstructionDecoder<"rl l", 0x15, Rl<RegisterType::L, false, true>, 0xCB>>::Type;
using RL_IndirectHL_Decoder  = Instantiate<InstructionDecoder<"rl [hl]", 0x16, Rl<RegisterType::HL, false, true>, 0xCB>>::Type;
using RL_A_Decoder           = Instantiate<InstructionDecoder<"rl a", 0x17, Rl<RegisterType::A, false, true>, 0xCB>>::Type;

using RR_B_Decoder           = Instantiate<InstructionDecoder<"rr b", 0x18, Rr<RegisterType::B, false, true>, 0xCB>>::Type;
using RR_C_Decoder           = Instantiate<InstructionDecoder<"rr c", 0x19, Rr<RegisterType::C, false, true>, 0xCB>>::Type;
using RR_D_Decoder           = Instantiate<InstructionDecoder<"rr d", 0x1A, Rr<RegisterType::D, false, true>, 0xCB>>::Type;
using RR_E_Decoder           = Instantiate<InstructionDecoder<"rr e", 0x1B, Rr<RegisterType::E, false, true>, 0xCB>>::Type;
using RR_H_Decoder           = Instantiate<InstructionDecoder<"rr h", 0x1C, Rr<RegisterType::H, false, true>, 0xCB>>::Type;
using RR_L_Decoder           = Instantiate<InstructionDecoder<"rr l", 0x1D, Rr<RegisterType::L, false, true>, 0xCB>>::Type;
using RR_IndirectHL_Decoder  = Instantiate<InstructionDecoder<"rr [hl]", 0x1E, Rr<RegisterType::HL, false, true>, 0xCB>>::Type;
using RR_A_Decoder           = Instantiate<InstructionDecoder<"rr a", 0x1F, Rr<RegisterType::A, false, true>, 0xCB>>::Type;

} // namespace pgb::cpu::instruction
