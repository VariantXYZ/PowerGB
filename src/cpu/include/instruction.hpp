#pragma once

#include <cstddef>
#include <tuple>

#include <common/result.hpp>
#include <cpu/include/registers.hpp>
#include <memory/include/memory.hpp>

namespace pgb::cpu
{
// An "instruction" is a set operations on state that takes some fixed duration
// Here, it is defined as:
// * A set of 'Operands'
// * A set of 'Operations' (one per tick) that may affect the memory state

template <common::ResultSetType RS>
class Operation
{
public:
    using FunctionType = RS (*)(memory::MemoryMap&);
    FunctionType _fn;

    constexpr Operation(FunctionType fn) noexcept : _fn(fn) {}

    constexpr RS operator()(memory::MemoryMap& mmap) const noexcept
    {
        return _fn(mmap);
    }
};

enum IncrementMode
{
    None = 0,
    Increment,
    Decrement
};

// Define common operaiions

using IncrementPCResultSet  = memory::MemoryMap::BaseAccessResultSet<void, memory::MemoryMap::ResultRegisterOverflow>;
using IncrementRegResultSet = memory::MemoryMap::BaseRegisterAccessResultSet<void>;
using LoadRegResultSet      = memory::MemoryMap::BaseAccessResultSet<void, memory::MemoryMap::ResultAccessRegisterInvalidWidth>;

// nop
template <common::ResultSetType R>
inline R NoOp(memory::MemoryMap&) noexcept
{
    return R::DefaultResultSuccess();
}

// PC++
inline IncrementPCResultSet IncrementPC(memory::MemoryMap& mmap) noexcept
{
    auto pcIncResult = mmap.IncrementPC();
    return pcIncResult;
}

// Reg++ or Reg--
template <RegisterType T, IncrementMode Mode>
inline IncrementRegResultSet SingleStepRegister(memory::MemoryMap& mmap) noexcept
{
    if (Mode == IncrementMode::None)
    {
        return IncrementRegResultSet::DefaultResultSuccess();
    }

    auto getResult = mmap.ReadWord(T);
    if (getResult.IsSuccess())
    {
        auto value     = static_cast<const Word>(getResult) + (Mode == IncrementMode::Increment ? 1 : -1);
        auto setResult = mmap.WriteWord(T, value);
        return setResult;
    }
    return getResult;
}

// Temp++ or Temp--
template <IncrementMode Mode>
inline IncrementRegResultSet SingleStepTemp(memory::MemoryMap& mmap) noexcept
{
    if (Mode == IncrementMode::None)
    {
        return IncrementRegResultSet::DefaultResultSuccess();
    }

    auto wz = mmap.GetTemp();
    wz++;
    mmap.GetTempHi() = wz.HighByte();
    mmap.GetTempLo() = wz.LowByte();
}

// IR <- [PC]
inline LoadRegResultSet LoadIRPC(memory::MemoryMap& mmap) noexcept
{
    auto pc     = mmap.ReadPC();
    auto result = mmap.ReadByte(pc);
    if (result.IsSuccess())
    {
        mmap.WriteByte(cpu::RegisterType::IR, result);
    }
    return result;
}

// Z <- [PC];
inline LoadRegResultSet LoadTempLoPC(memory::MemoryMap& mmap) noexcept
{
    auto pc     = mmap.ReadPC();
    auto result = mmap.ReadByte(pc);
    if (result.IsSuccess())
    {
        auto& Z = mmap.GetTempLo();
        Z       = result;
    }
    return result;
}
// W <- [PC];
inline LoadRegResultSet LoadTempHiPC(memory::MemoryMap& mmap) noexcept
{
    auto pc     = mmap.ReadPC();
    auto result = mmap.ReadByte(pc);
    if (result.IsSuccess())
    {
        auto& W = mmap.GetTempHi();
        W       = result;
    }
    return result;
}
// Z <- [WZ];
inline LoadRegResultSet LoadTempLoTemp(memory::MemoryMap& mmap) noexcept
{
    auto wz     = mmap.GetTemp();
    auto result = mmap.ReadByte(wz);
    if (result.IsSuccess())
    {
        auto& Z = mmap.GetTempLo();
        Z       = result;
    }
    return result;
}

// Z -> Reg8
template <auto Destination>
    requires(IsRegister8Bit<Destination>)
inline LoadRegResultSet LoadReg8TempLo(memory::MemoryMap& mmap) noexcept
{
    auto result = mmap.WriteByte(Destination, mmap.GetTempLo());
    return result;
}

// Z -> [Reg16]
template <auto Destination>
    requires(IsRegister16Bit<Destination>)
inline LoadRegResultSet LoadReg8TempLoIndirect(memory::MemoryMap& mmap) noexcept
{
    auto dstAddr = mmap.ReadWord(Destination);
    if (dstAddr.IsFailure())
    {
        return dstAddr;
    }
    const Word w = static_cast<Word>(dstAddr);
    return mmap.WriteByte(static_cast<std::uint_fast16_t>(w), mmap.GetTempLo());
}

// WZ -> Reg16
template <auto Destination>
    requires(IsRegister16Bit<Destination>)
inline LoadRegResultSet LoadReg16Temp(memory::MemoryMap& mmap) noexcept
{
    auto result = mmap.WriteWord(Destination, mmap.GetTemp());
    return result;
}

// [WZ] <- Reg16 (2 bytes)
template <RegisterType Source>
    requires(IsRegister16Bit<Source>)
inline LoadRegResultSet LoadTempIndirect(memory::MemoryMap& mmap) noexcept
{
    auto wz     = mmap.GetTemp();
    auto result = mmap.ReadWord(Source);
    if (result.IsSuccess())
    {
        return mmap.WriteWordLE(wz, result);
    }
    return result;
}

// [WZ] <- Reg8
template <RegisterType Source>
    requires(IsRegister8Bit<Source>)
inline LoadRegResultSet LoadTempIndirect(memory::MemoryMap& mmap) noexcept
{
    auto wz     = mmap.GetTemp();
    auto result = mmap.ReadByte(Source);
    if (result.IsSuccess())
    {
        return mmap.WriteByte(wz, result);
    }
    return result;
}

// Imm8 -> Z
template <bool IsTempHi, const Byte Source>
inline LoadRegResultSet LoadTempImm8(memory::MemoryMap& mmap) noexcept
{
    auto& t = IsTempHi ? mmap.GetTempHi() : mmap.GetTempLo();
    t = Source;
    return LoadRegResultSet::DefaultResultSuccess();
}

template <std::size_t Ticks_, Operation... Operations>
class Instruction
{
private:
    using OperationVisitor                          = bool (*)(memory::MemoryMap&) noexcept;
    static constexpr OperationVisitor _operations[] = {[](memory::MemoryMap& memory) noexcept
                                                       { return Operations(memory).IsSuccess(); }...};

public:
    static constexpr std::size_t Ticks = Ticks_;
    Instruction()                      = delete;

    // Execute all operations until a failure occurs
    // Returns the failing operation index, otherwise the total number of ticks expected
    constexpr static std::size_t ExecuteAll(memory::MemoryMap& memory) noexcept
    {
        std::size_t t = 0;
        (void)((Operations(memory).IsSuccess() ? (++t, true) : false) && ...);
        return t == sizeof...(Operations) ? Ticks : t;
    }

    // Execute all operations ignoring the result value
    // Ignoring the results should allow the compiler to more heavily inline everything
    constexpr static void ExecuteAllForce(memory::MemoryMap& memory) noexcept
    {
        (void)((Operations(memory)), ...);
    }

    // Execute specific operation
    template <std::size_t T>
    constexpr static auto ExecuteCycle(memory::MemoryMap& memory) noexcept
        requires(T < sizeof...(Operations))
    {
        return std::get<T>(std::forward_as_tuple(Operations...))(memory);
    }

    // Execute specific operation, does not retain the result value
    constexpr static bool ExecuteCycle(memory::MemoryMap& memory, std::size_t T) noexcept
    {
        return _operations[T](memory);
    }
};

template <class I>
concept InstructionType =
    requires(I i) {
        {
            Instruction{i}
        } -> std::same_as<I>;
    };

} // namespace pgb::cpu
