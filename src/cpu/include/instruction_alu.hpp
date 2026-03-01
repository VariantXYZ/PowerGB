#pragma once

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
    requires(IsRegister8Bit<Destination> && IsRegister8Bit<Operand>)
inline BasicAluResultSet AddReg(MemoryMap& mmap) noexcept
{
    auto dst = mmap.ReadByte(Destination);
    if (dst.IsFailure())
    {
        return dst;
    }

    auto src = mmap.ReadByte(Operand);
    if (src.IsFailure())
    {
        return src;
    }

    // Note that src value is specifically not a reference
    bool         Z, N, H, C;
    auto         srcValue    = static_cast<const Byte>(src);
    auto&        dstValue    = static_cast<const Byte&>(dst);
    unsigned int value       = srcValue + dstValue;
    auto         valueCasted = static_cast<Byte>(value);
    auto         result      = mmap.WriteByte(Destination, static_cast<Byte>(valueCasted));

    // Flags
    Z                        = valueCasted == 0;
    N                        = false;
    H                        = (srcValue.LowNibble() + dstValue.LowNibble()) > 0xF;
    C                        = (valueCasted < dstValue) || (valueCasted < srcValue);
    Nibble flag              = Z << 3 | N << 2 | H << 1 | C << 0;
    mmap.WriteFlag(flag);

    return result;
}

template <auto Destination, auto Operand, std::size_t Ticks = 4>
    requires(IsRegister8Bit<Destination> && IsRegister8Bit<Operand>)
using Add = Instruction<
    /*Ticks*/ Ticks,
    AddReg<Destination, Operand>,
    IncrementPC,
    LoadIRPC>;

using Add_A_B_Decoder = Instantiate<InstructionDecoder<"add a, b", 0x80, Add<RegisterType::A, RegisterType::B>>>::Type;

} // namespace pgb::cpu::instruction
