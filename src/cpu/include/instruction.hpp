#pragma once

#include <cstddef>
#include <tuple>

#include <common/result.hpp>
#include <common/util.hpp>
#include <cpu/include/registers.hpp>
#include <memory/include/memory.hpp>

namespace pgb::cpu
{

// An "instruction" is a set operations on state that takes some fixed duration
// Here, it is defined as:
// * A set of 'Operands'
// * A set of 'Operations' (one per tick) that may affect the memory state
template <typename Op>
concept Operation = common::ReturnsResultSet<Op::Execute, memory::MemoryMap&>;

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
struct NoOp
{
    static inline R Execute(memory::MemoryMap&) noexcept
    {
        return R::DefaultResultSuccess();
    }
};

// PC++
struct IncrementPC
{
    static inline IncrementPCResultSet Execute(memory::MemoryMap& mmap) noexcept
    {
        auto pcIncResult = mmap.IncrementPC();
        return pcIncResult;
    }
};

// IR <- [PC]
struct LoadIRPC
{
    static inline LoadRegResultSet Execute(memory::MemoryMap& mmap) noexcept
    {
        auto pc     = mmap.ReadPC();
        auto result = mmap.ReadByte(pc);
        if (result.IsSuccess())
        {
            mmap.WriteByte(cpu::RegisterType::IR, result);
        }
        return result;
    }
};

// Reg++ or Reg--
template <RegisterType T, IncrementMode Mode>
struct SingleStepRegister
{
    static inline IncrementRegResultSet Execute(memory::MemoryMap& mmap) noexcept
        requires(IsRegister16Bit<T>)
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

    static inline IncrementRegResultSet Execute(memory::MemoryMap& mmap) noexcept
        requires(IsRegister8Bit<T>)
    {
        if (Mode == IncrementMode::None)
        {
            return IncrementRegResultSet::DefaultResultSuccess();
        }

        auto getResult = mmap.ReadByte(T);
        if (getResult.IsSuccess())
        {
            auto value     = static_cast<const Byte>(getResult) + (Mode == IncrementMode::Increment ? 1 : -1);
            auto setResult = mmap.WriteByte(T, value);
            return setResult;
        }
        return getResult;
    }
};

// Temp++ or Temp--
template <IncrementMode Mode>
struct SingleStepTemp
{
    static inline IncrementRegResultSet Execute(memory::MemoryMap& mmap) noexcept
    {
        if (Mode == IncrementMode::None)
        {
            return IncrementRegResultSet::DefaultResultSuccess();
        }

        auto wz = mmap.GetTemp();
        wz++;
        mmap.GetTempHi() = wz.HighByte();
        mmap.GetTempLo() = wz.LowByte();

        return IncrementRegResultSet::DefaultResultSuccess();
    }
};

// Temp8 <- [PC];
template <bool IsTempHi>
struct LoadTemp8PC
{
    static inline LoadRegResultSet Execute(memory::MemoryMap& mmap) noexcept
    {
        auto pc     = mmap.ReadPC();
        auto result = mmap.ReadByte(pc);
        if (result.IsSuccess())
        {
            auto& x = IsTempHi ? mmap.GetTempHi() : mmap.GetTempLo();
            x       = result;
        }
        return result;
    }
};
// Z <- [PC]
using LoadTempLoPC = LoadTemp8PC<false>;
// W <- [PC];
using LoadTempHiPC = LoadTemp8PC<true>;

// Z <- [WZ];
struct LoadTempLoTemp
{
    static inline LoadRegResultSet Execute(memory::MemoryMap& mmap) noexcept
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
};

// Z -> Reg8
template <auto Destination>
    requires(IsRegister8Bit<Destination>)
struct LoadReg8TempLo
{
    static inline LoadRegResultSet Execute(memory::MemoryMap& mmap) noexcept
    {
        auto result = mmap.WriteByte(Destination, mmap.GetTempLo());
        return result;
    }
};

// Z -> [Reg16]
template <auto Destination>
    requires(IsRegister16Bit<Destination>)
struct LoadReg8TempLoIndirect
{
    static inline LoadRegResultSet Execute(memory::MemoryMap& mmap) noexcept
    {
        auto dstAddr = mmap.ReadWord(Destination);
        if (dstAddr.IsFailure())
        {
            return dstAddr;
        }
        const Word w = static_cast<Word>(dstAddr);
        return mmap.WriteByte(static_cast<std::uint_fast16_t>(w), mmap.GetTempLo());
    }
};

// Z <- [Reg16]
template <auto Source>
    requires(IsRegister16Bit<Source>)
struct LoadIndirectReg8TempLo
{
    static inline LoadRegResultSet Execute(memory::MemoryMap& mmap) noexcept
    {
        auto srcAddr = mmap.ReadWord(Source);
        if (srcAddr.IsFailure())
        {
            return srcAddr;
        }

        const Word w      = static_cast<Word>(srcAddr);
        auto       result = mmap.ReadByte(w);

        if (result.IsFailure())
        {
            return result;
        }

        auto& z = mmap.GetTempLo();
        z       = static_cast<Byte>(result);

        return LoadRegResultSet::DefaultResultSuccess();
    }
};

// WZ -> Reg16
template <auto Destination>
    requires(IsRegister16Bit<Destination>)
struct LoadReg16Temp
{
    static inline LoadRegResultSet Execute(memory::MemoryMap& mmap) noexcept
    {
        auto result = mmap.WriteWord(Destination, mmap.GetTemp());
        return result;
    }
};

// Reg16 -> WZ
template <auto Source>
    requires(IsRegister16Bit<Source>)
struct LoadTempReg16
{
    static inline LoadRegResultSet Execute(memory::MemoryMap& mmap) noexcept
    {
        auto r1 = mmap.ReadWord(Source);
        if (r1.IsFailure())
        {
            return r1;
        }
        auto& val        = static_cast<const Word&>(r1);
        mmap.GetTempHi() = val.HighByte();
        mmap.GetTempLo() = val.LowByte();
        return r1;
    }
};

template <RegisterType Source>
struct LoadTempIndirect
{
    // [WZ] <- Reg16 (2 bytes)
    static inline LoadRegResultSet Execute(memory::MemoryMap& mmap) noexcept
        requires(IsRegister16Bit<Source>)
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
    static inline LoadRegResultSet Execute(memory::MemoryMap& mmap) noexcept
        requires(IsRegister8Bit<Source>)
    {
        auto wz     = mmap.GetTemp();
        auto result = mmap.ReadByte(Source);
        if (result.IsSuccess())
        {
            return mmap.WriteByte(wz, result);
        }
        return result;
    }
};

// Imm8 -> Z
template <bool IsTempHi, const Byte Source>
struct LoadTempImm8
{
    static inline LoadRegResultSet Execute(memory::MemoryMap& mmap) noexcept
    {
        auto& t = IsTempHi ? mmap.GetTempHi() : mmap.GetTempLo();
        t       = Source;
        return LoadRegResultSet::DefaultResultSuccess();
    }
};

// Reg8 -> Temp
template <bool IsTempHi, RegisterType Source>
    requires(IsRegister8Bit<Source>)
struct LoadTempReg8
{
    static inline LoadRegResultSet Execute(memory::MemoryMap& mmap) noexcept
    {
        auto& t = IsTempHi ? mmap.GetTempHi() : mmap.GetTempLo();
        auto  r = mmap.ReadByte(Source);
        if (r.IsSuccess())
        {
            t = static_cast<const Byte&>(r);
        }
        return r;
    }
};

template <typename T>
concept HasAlternativeTicks = requires {
    // Requires that T::size is a valid expression and can be converted to size_t
    { T::AlternativeTicks } -> std::convertible_to<std::size_t>;
};

// Use NTTP for supporting base/alternative ticks
struct TicksType
{
    std::size_t BaseTicks;
    std::size_t AlternativeTicks;

    consteval TicksType(std::size_t base, std::size_t alternative = 0)
    {
        BaseTicks        = base;
        AlternativeTicks = alternative;
    }
};

template <TicksType Ticks_, Operation... Operations>
class Instruction
{
private:
    using OperationVisitor                          = bool (*)(memory::MemoryMap&) noexcept;
    static constexpr OperationVisitor _operations[] = {[](memory::MemoryMap& memory) noexcept
                                                       { return Operations::Execute(memory).IsSuccess(); }...};

public:
    // An easy way to check lengths is to just see how many times we call IncrementPC (just use its type to figure it out)
    static constexpr std::size_t Length           = ((std::is_same_v<decltype(Operations::Execute(std::declval<memory::MemoryMap&>())), IncrementPCResultSet> ? 1 : 0) + ...);

    static constexpr std::size_t Ticks            = Ticks_.BaseTicks;
    static constexpr std::size_t AlternativeTicks = Ticks_.AlternativeTicks;

    Instruction()                                 = delete;

    // Execute all operations until a failure occurs
    // Returns the failing operation index, otherwise the total number of ticks expected
    template <bool Force = false>
    constexpr static std::size_t ExecuteAll(memory::MemoryMap& memory) noexcept
    {
        if constexpr (Force)
        {
            (void)((Operations::Execute(memory)), ...);

            if constexpr (AlternativeTicks != 0)
            {
                if (memory.UseAlternativeTicks()) [[unlikely]]
                {
                    return AlternativeTicks;
                }
            }
            return Ticks;
        }
        else
        {
            std::size_t t = 0;
            (void)((Operations::Execute(memory).IsSuccess() ? (++t, true) : false) && ...);

            if (t == sizeof...(Operations)) [[likely]]
            {
                if constexpr (AlternativeTicks != 0)
                {
                    if (memory.UseAlternativeTicks()) [[unlikely]]
                    {
                        return AlternativeTicks;
                    }
                }
                return Ticks;
            }
            else
            {
                return t;
            }
        }
    }

    // Execute specific operation
    template <std::size_t T>
    constexpr static auto ExecuteCycle(memory::MemoryMap& memory) noexcept
        requires(T < sizeof...(Operations))
    {
        return std::get<T>(std::forward_as_tuple(Operations::Execute...))(memory);
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
