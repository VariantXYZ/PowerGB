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

using RLCA_Decoder = Instantiate<InstructionDecoder<"rlca", 0x07, Rl<RegisterType::A, true, false>>>::Type;
using RRCA_Decoder = Instantiate<InstructionDecoder<"rrca", 0x0F, Rr<RegisterType::A, true, false>>>::Type;
using RLA_Decoder  = Instantiate<InstructionDecoder<"rla", 0x17, Rl<RegisterType::A, false, false>>>::Type;
using RRA_Decoder  = Instantiate<InstructionDecoder<"rra", 0x1F, Rr<RegisterType::A, false, false>>>::Type;

// CB Prefix
using RLC_B_Decoder = Instantiate<InstructionDecoder<"rlc b", 0x00, Rl<RegisterType::B, true, true>, 0xCB>>::Type;
static_assert(std::is_same_v<RLC_B_Decoder::RegistryType, instruction::detail::InstructionRegistryTagPrefixCB>);
static_assert(instruction::InstructionRegistryPrefixCB::Lengths[0x00] > 0);

} // namespace pgb::cpu::instruction
