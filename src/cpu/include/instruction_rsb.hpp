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
inline BasicRsbResultSet RotateLeft(MemoryMap& mmap) noexcept
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

template <RegisterType Destination, bool Circular, bool Zero>
inline BasicRsbResultSet RotateRight(MemoryMap& mmap) noexcept
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

template <RegisterType Destination, bool Circular, bool Zero>
using Rl = Instruction<
    /*Ticks*/ 4,
    RotateLeft<Destination, Circular, Zero>,
    IncrementPC,
    LoadIRPC>;

template <RegisterType Destination, bool Circular, bool Zero>
using Rr = Instruction<
    /*Ticks*/ 4,
    RotateRight<Destination, Circular, Zero>,
    IncrementPC,
    LoadIRPC>;

using RLCA_Decoder  = Instantiate<InstructionDecoder<"rlca", 0x07, Rl<RegisterType::A, true, false>>>::Type;
using RRCA_Decoder  = Instantiate<InstructionDecoder<"rrca", 0x0F, Rr<RegisterType::A, true, false>>>::Type;
using RLA_Decoder   = Instantiate<InstructionDecoder<"rla", 0x17, Rl<RegisterType::A, false, false>>>::Type;
using RRA_Decoder   = Instantiate<InstructionDecoder<"rra", 0x1F, Rr<RegisterType::A, false, false>>>::Type;

// CB Prefix
using RLC_B_Decoder = Instantiate<InstructionDecoder<"rlc b", 0x00, Rl<RegisterType::B, true, true>, 0xCB>>::Type;
using RLC_C_Decoder = Instantiate<InstructionDecoder<"rlc c", 0x01, Rl<RegisterType::C, true, true>, 0xCB>>::Type;
using RLC_D_Decoder = Instantiate<InstructionDecoder<"rlc d", 0x02, Rl<RegisterType::D, true, true>, 0xCB>>::Type;
using RLC_E_Decoder = Instantiate<InstructionDecoder<"rlc e", 0x03, Rl<RegisterType::E, true, true>, 0xCB>>::Type;
using RLC_H_Decoder = Instantiate<InstructionDecoder<"rlc h", 0x04, Rl<RegisterType::H, true, true>, 0xCB>>::Type;
using RLC_L_Decoder = Instantiate<InstructionDecoder<"rlc l", 0x05, Rl<RegisterType::L, true, true>, 0xCB>>::Type;

using RL_B_Decoder  = Instantiate<InstructionDecoder<"rl b", 0x10, Rl<RegisterType::B, false, true>, 0xCB>>::Type;
using RL_C_Decoder  = Instantiate<InstructionDecoder<"rl c", 0x11, Rl<RegisterType::C, false, true>, 0xCB>>::Type;
using RL_D_Decoder  = Instantiate<InstructionDecoder<"rl d", 0x12, Rl<RegisterType::D, false, true>, 0xCB>>::Type;
using RL_E_Decoder  = Instantiate<InstructionDecoder<"rl e", 0x13, Rl<RegisterType::E, false, true>, 0xCB>>::Type;
using RL_H_Decoder  = Instantiate<InstructionDecoder<"rl h", 0x14, Rl<RegisterType::H, false, true>, 0xCB>>::Type;
using RL_L_Decoder  = Instantiate<InstructionDecoder<"rl l", 0x15, Rl<RegisterType::L, false, true>, 0xCB>>::Type;

} // namespace pgb::cpu::instruction
