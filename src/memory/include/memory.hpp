#pragma once

#include <bit>
#include <cstdint>

#include <common/block.hpp>
#include <common/datatypes.hpp>
#include <common/result.hpp>

namespace pgb::memory
{

// Pre-emptively statically allocate all necessary buffers
constexpr static std::size_t MaxBankValue     = 0x1FF;
constexpr static std::size_t MaxAddressValue  = 0xFFFF;

constexpr static std::size_t MaxRomBankCount  = 0x200;
constexpr static std::size_t MaxVramBankCount = 2;
constexpr static std::size_t MaxEramBankCount = 2;
constexpr static std::size_t MaxWramBankCount = 8;

// Function results
using ResultAccessInvalidBank                 = common::Result<"Bank not in valid range">;
using ResultAccessInvalidAddress              = common::Result<"Address not in valid range">;
using ResultAccessProhibitedAddress           = common::Result<"Accessing vendor prohibited address">;

using namespace pgb::common::block;
using namespace pgb::common::datatypes;

// GB has a 16-bit address space to map IO, ROM, & RAM
// This class provides an interface to the raw underlying memory representation of this address space.
// The regions are statically allocated and this memory map simply allows for address restrictions.
class MemoryMap
{
private:
    const std::size_t _romBankCount  = MaxRomBankCount;
    const std::size_t _vramBankCount = MaxVramBankCount;
    const std::size_t _eramBankCount = MaxEramBankCount;
    const std::size_t _wramBankCount = MaxWramBankCount;
    bool              _isInitialized = false;

public:
    struct MemoryAddress
    {
        uint_fast16_t bank : std::bit_width(static_cast<uint_fast16_t>(MaxBankValue));
        uint_fast16_t address : std::bit_width(static_cast<uint_fast16_t>(MaxAddressValue));
    };
    constexpr bool IsInitialized() { return _isInitialized; }

    constexpr MemoryMap() noexcept {}

    constexpr MemoryMap(std::size_t romBankCount, std::size_t vramBankCount, std::size_t eramBankCount, std::size_t wramBankCount) noexcept
        : _romBankCount(romBankCount),
          _vramBankCount(vramBankCount),
          _eramBankCount(eramBankCount),
          _wramBankCount(wramBankCount) {}

    using InitializeResultSet = common::ResultSet<bool, common::ResultSuccess, common::ResultFailure>;
    InitializeResultSet Initialize(const Byte (&rom)[]) noexcept;

    using AccessResultSet = common::ResultSet<const Byte&, common::ResultSuccess, ResultAccessInvalidBank, ResultAccessInvalidAddress, ResultAccessProhibitedAddress>;
    // Access a byte at a specific address, the stored result is only valid if it is marked successful
    AccessResultSet AccessByte(const MemoryAddress&) const noexcept;

    // Write a byte at a specific address if it is accessible and returns the value written.
    // If the address is not accessible, the function will propagate the AccessByte error.
    // Note that this function will write as long as the address is within a valid range.
    AccessResultSet WriteByte(const MemoryAddress&, const Byte& value) noexcept;
};

} // namespace pgb::memory