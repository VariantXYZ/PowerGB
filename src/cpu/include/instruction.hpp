#pragma once

#include <tuple>
#include <type_traits>

#include <common/result.hpp>
#include <cpu/include/registers.hpp>
#include <memory/include/memory.hpp>

namespace pgb::cpu
{
// An "instruction" is a set operations on state that takes some fixed duration
// This class aims to provide some representation for the necessary metadata
// So we must note:
// * The number of clock cycles it takes to execute
// * The actual change it makes at each cycle

// An instruction is defined by:
// * A set of 'Operands'
// * A set of 'Operation' representing an action at each cycle that takes those operands and the MemoryMap as input

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

template <Operation... Operations>
class Instruction
{
public:
    static constexpr std::size_t Cycles = sizeof...(Operations);
    Instruction()                       = delete;

    // Execute all operations, disregarding the result status
    constexpr static void ExecuteAll(memory::MemoryMap& memory) noexcept
    {
        (Operations(memory), ...);
    }

    // Execute specific cycle
    template <std::size_t T>
    constexpr static auto ExecuteCycle(memory::MemoryMap& memory) noexcept
        requires(T < Cycles)
    {
        return std::get<T>(std::forward_as_tuple(Operations...))(memory);
    }
};

} // namespace pgb::cpu