#pragma once

#include <bit>
#include <cstdint>

#include <common/block.hpp>
#include <common/datatypes.hpp>
#include <common/result.hpp>
#include <cpu/include/cpu.hpp>

namespace pgb::memory
{

// Pre-emptively statically allocate all necessary buffers
constexpr static std::size_t MaxBankValue     = 0x1FF;
constexpr static std::size_t MaxAddressValue  = 0xFFFF;

constexpr static std::size_t MaxRomBankCount  = 0x200;
constexpr static std::size_t MaxVramBankCount = 2;
constexpr static std::size_t MaxEramBankCount = 2;
constexpr static std::size_t MaxWramBankCount = 8;

constexpr static std::size_t RomBankSize      = 0x4000;

// Function results
//// Access
using ResultAccessInvalidBank                 = common::Result<"Bank not in valid range">;
using ResultAccessInvalidAddress              = common::Result<"Address not in valid range">;
using ResultAccessProhibitedAddress           = common::Result<"Accessing vendor prohibited address">;

//// Initialization
using ResultInitializeInvalidAlignment        = common::Result<"ROM size is not a multiple of 0x4000">;

using namespace pgb::common::block;
using namespace pgb::common::datatypes;

// GB has a 16-bit address space to map IO, ROM, & RAM
// This class provides an interface to the raw underlying memory representation of this address space.
// Note that this class is generally too large to be allocated on the stack.
class MemoryMap
{
private:
    // The bus maps references to the CPU IE
    const cpu::CPU& _cpu;

    std::size_t _romBankCount;
    std::size_t _vramBankCount;
    std::size_t _eramBankCount;
    std::size_t _wramBankCount;

    // IO Registers should be handled as individual registers
    // We could be more memory efficient, but doing this allows for statically allocating this entire class
    Block<RomBankSize * Byte::TypeWidth, Byte::TypeWidth> _rom[MaxRomBankCount];
    Block<0x2000 * Byte::TypeWidth, Byte::TypeWidth>      _vram[MaxVramBankCount];
    Block<0x2000 * Byte::TypeWidth, Byte::TypeWidth>      _eram[MaxEramBankCount];
    Block<0x1000 * Byte::TypeWidth, Byte::TypeWidth>      _wram[MaxWramBankCount];
    Block<0xA0 * Byte::TypeWidth, Byte::TypeWidth>        _oam;
    Block<0x80 * Byte::TypeWidth, Byte::TypeWidth>        _hram;
    Block<0x80 * Byte::TypeWidth, Byte::TypeWidth>        _io;

    bool _isInitialized = false;

public:
    struct MemoryAddress
    {
        uint_fast16_t bank : std::bit_width(static_cast<uint_fast16_t>(MaxBankValue));
        uint_fast16_t address : std::bit_width(static_cast<uint_fast16_t>(MaxAddressValue));
    };
    constexpr bool IsInitialized() { return _isInitialized; }

    constexpr MemoryMap(cpu::CPU& cpu, std::size_t romBankCount, std::size_t vramBankCount, std::size_t eramBankCount, std::size_t wramBankCount) noexcept
        : _cpu(cpu),
          _romBankCount(romBankCount),
          _vramBankCount(vramBankCount),
          _eramBankCount(eramBankCount),
          _wramBankCount(wramBankCount) {}

    using InitializeResultSet = common::ResultSet<void, common::ResultSuccess, ResultInitializeInvalidAlignment>;
    InitializeResultSet Initialize(const Byte (&rom)[], const std::size_t size) noexcept;

    using AccessResultSet = common::ResultSet<const Byte&, common::ResultSuccess, ResultAccessInvalidBank, ResultAccessInvalidAddress, ResultAccessProhibitedAddress>;

    // Access a byte at a specific address, the stored result is only valid if it is marked successful.
    // ResultAccessInvalidBank is always a failure case
    // ResultAccessInvalidAddress is always a failure case
    // ResultAccessProhibitedAddress is sometimes returned as a failure case
    AccessResultSet AccessByte(const MemoryAddress&) const noexcept;

    // Write a byte at a specific address if it is accessible and returns the previous value.
    // If the address is not accessible, the function will propagate the AccessByte error.
    // Note that this function will write as long as the address is within a valid range.
    AccessResultSet WriteByte(const MemoryAddress&, const Byte& value) noexcept;
};

} // namespace pgb::memory