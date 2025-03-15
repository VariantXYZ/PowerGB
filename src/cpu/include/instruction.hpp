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
// * A set of 'Operations' (one per tick) that may affects the memory state

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
NoOpResultSet NoOp(memory::MemoryMap&) noexcept
{
    return NoOpResultSet::DefaultResultSuccess();
}

template <Operation... Operations>
class Instruction
{
private:
    using OperationVisitor                          = bool (*)(memory::MemoryMap&);
    static constexpr OperationVisitor _operations[] = {[](memory::MemoryMap& memory)
                                                       { return Operations(memory).IsSuccess(); }...};
public:
    static constexpr std::size_t Ticks = sizeof...(Operations);
    Instruction()                      = delete;

    // Execute all operations until a failure occurs
    constexpr static std::size_t ExecuteAll(memory::MemoryMap& memory) noexcept
    {
        std::size_t t = 0;
        (void)((Operations(memory).IsSuccess() ? (++t, true) : false) && ...);
        return t;
    }

    // Execute specific cycle
    template <std::size_t T>
    constexpr static auto ExecuteCycle(memory::MemoryMap& memory) noexcept
        requires(T < Ticks)
    {
        return std::get<T>(std::forward_as_tuple(Operations...))(memory);
    }

    // Execute specific cycle, does not retain the result value
    constexpr static bool ExecuteCycle(memory::MemoryMap& memory, std::size_t T) noexcept
    {
        return _operations[T](memory);
    }
};

} // namespace pgb::cpu