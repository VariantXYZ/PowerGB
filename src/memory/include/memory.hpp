#pragma once

#include <bit>
#include <cstdint>

#include <common/block.hpp>
#include <common/datatypes.hpp>
#include <common/result.hpp>
#include <cpu/include/registers.hpp>

using namespace pgb::common::block;
using namespace pgb::common::datatypes;

namespace pgb::memory
{

constexpr static const std::size_t ValidRomBankCount[]         = {2, 4, 8, 16, 32, 64, 128, 256, 512, 72, 80, 96};
constexpr static const std::size_t ValidExternalRamBankCount[] = {0, 1, 4, 16, 8};

// Pre-emptively statically allocate all necessary buffers
constexpr static std::size_t MaxBankValue                      = 0x1FF;
constexpr static std::size_t MaxAddressValue                   = 0xFFFF;

constexpr static std::size_t MaxRomBankCount                   = 0x200;
constexpr static std::size_t MaxVramBankCount                  = 2;
constexpr static std::size_t MaxEramBankCount                  = 16;
constexpr static std::size_t MaxWramBankCount                  = 8;

constexpr static std::size_t RomBankSize                       = 0x4000;
constexpr static std::size_t VramBankSize                      = 0x2000;
constexpr static std::size_t EramBankSize                      = 0x2000;
constexpr static std::size_t WramBankSize                      = 0x1000;

// GB has a 16-bit address space to map IO, ROM, & RAM
// This class provides an interface to the raw underlying memory representation of this address space.
// Note that this class is generally too large to be allocated on the stack.
class MemoryMap
{
public:
    // Function results
    //// Access
    using ResultAccessInvalidBank       = common::Result<"Bank not in valid range">;
    using ResultAccessInvalidAddress    = common::Result<"Address not in valid range">;
    using ResultAccessProhibitedAddress = common::Result<"Accessing vendor prohibited address">;
    using AccessResultSet =
        common::ResultSet<
            /* Type */ const Byte&,
            common::ResultSuccess,
            ResultAccessInvalidBank,
            ResultAccessInvalidAddress,
            ResultAccessProhibitedAddress>;

    //// Initialization
    using ResultInitializeInvalidAlignment     = common::Result<"ROM size is not a multiple of 0x4000">;
    using ResultInitializeAlreadyInitialized   = common::Result<"Memory has already been initialized">;
    using ResultInitializeInvalidRomBankCount  = common::Result<"ROM bank count is invalid">;
    using ResultInitializeInvalidVramBankCount = common::Result<"VRAM bank count is invalid">;
    using ResultInitializeInvalidEramBankCount = common::Result<"ERAM bank count is invalid">;
    using ResultInitializeInvalidWramBankCount = common::Result<"WRAM bank count is invalid">;
    using InitializeResultSet =
        common::ResultSet<
            /* Type */ void,
            common::ResultSuccess,
            ResultInitializeInvalidAlignment,
            ResultInitializeAlreadyInitialized,
            ResultInitializeInvalidRomBankCount,
            ResultInitializeInvalidVramBankCount,
            ResultInitializeInvalidEramBankCount,
            ResultInitializeInvalidWramBankCount>;

private:
    // The bus maps references to the CPU IE
    const cpu::RegisterFile& _registers;

    std::size_t _romBankCount;
    std::size_t _vramBankCount;
    std::size_t _eramBankCount;
    std::size_t _wramBankCount;

    // IO Registers should be handled as individual registers
    // We could be more memory efficient, but doing this allows for statically allocating this entire class
    Block<RomBankSize * Byte::TypeWidth, Byte::TypeWidth>  _rom[MaxRomBankCount];
    Block<VramBankSize * Byte::TypeWidth, Byte::TypeWidth> _vram[MaxVramBankCount];
    Block<EramBankSize * Byte::TypeWidth, Byte::TypeWidth> _eram[MaxEramBankCount];
    Block<WramBankSize * Byte::TypeWidth, Byte::TypeWidth> _wram[MaxWramBankCount];
    Block<0xA0 * Byte::TypeWidth, Byte::TypeWidth>         _oam;
    Block<0x80 * Byte::TypeWidth, Byte::TypeWidth>         _hram;
    Block<0x80 * Byte::TypeWidth, Byte::TypeWidth>         _io;

    bool _isInitialized = false;

public:
    struct MemoryAddress
    {
        uint_fast16_t bank : std::bit_width(static_cast<uint_fast16_t>(MaxBankValue));
        uint_fast16_t address : std::bit_width(static_cast<uint_fast16_t>(MaxAddressValue));
    };

    constexpr MemoryMap(cpu::RegisterFile& registers) noexcept
        : _registers(registers) {}

    // Initialize memory based on an input ROM, will handle parsing the cartridge header to pull necessary metadata
    // All non-default results (anything that is not ResultSuccess) are considered failure cases and will not initialize the map
    InitializeResultSet Initialize(const Byte (&rom)[], const std::size_t size) noexcept;
    InitializeResultSet Initialize(std::size_t romBankCount, std::size_t vramBankCount, std::size_t eramBankCount, std::size_t wramBankCount) noexcept;

    // Sets all memory regions to 0
    void Reset() noexcept;

    constexpr bool IsInitialized() { return _isInitialized; }

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