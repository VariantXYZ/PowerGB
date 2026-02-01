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

using NoOpResultSet = common::ResultSet<void, common::ResultSuccess>;
// nop
inline NoOpResultSet NoOp(memory::MemoryMap&) noexcept
{
    return NoOpResultSet::DefaultResultSuccess();
}

using LoadIrResultSet = memory::MemoryMap::BaseAccessResultSet<void>;
// IR <- [PC]; ++PC;
inline LoadIrResultSet LoadIR(memory::MemoryMap& mmap) noexcept
{
    auto pc     = mmap.ReadPC();
    auto result = mmap.ReadByte(pc);
    if (result.IsSuccess())
    {
        mmap.WriteByte(cpu::RegisterType::IR, result);
        auto pcIncResult = mmap.IncrementPC();
        if (pcIncResult.IsFailure())
        {
            // TODO: Need to support result sets concatenating result sets so I can just add the result for this to it
            return LoadIrResultSet::DefaultResultFailure();
        }
    }
    return result;
}

template <std::size_t Ticks_, Operation... Operations>
class Instruction
{
private:
    using OperationVisitor                          = bool (*)(memory::MemoryMap&);
    static constexpr OperationVisitor _operations[] = {[](memory::MemoryMap& memory)
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

    // Execute specific operation
    template <std::size_t T>
    constexpr static auto ExecuteCycle(memory::MemoryMap& memory) noexcept
        requires(T < Ticks)
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
