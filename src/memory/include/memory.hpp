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
constexpr static const std::size_t ValidVramBankCount[]        = {1, 2};
constexpr static const std::size_t ValidExternalRamBankCount[] = {0, 1, 4, 16, 8};
constexpr static const std::size_t ValidWramBankCount[]        = {2, 8};

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
    using ResultAccessInvalidBank               = common::Result<"Bank not in valid range">;
    using ResultAccessInvalidAddress            = common::Result<"Address not in valid range">;
    using ResultAccessProhibitedAddress         = common::Result<"Accessing prohibited address">;
    using ResultAccessReadOnlyProhibitedAddress = common::Result<"Accessing read-only prohibited address">;
    using AccessResultSet =
        common::ResultSet<
            /* Type */ const Byte&,
            common::ResultSuccess,
            ResultAccessInvalidBank,
            ResultAccessInvalidAddress,
            ResultAccessProhibitedAddress,
            ResultAccessReadOnlyProhibitedAddress>;

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

    bool _isInitialized            = false;

    constexpr static const Byte _FEA0_FEFF[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

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
    constexpr InitializeResultSet Initialize(const Byte (&rom)[], const std::size_t size) noexcept;
    constexpr InitializeResultSet Initialize(std::size_t romBankCount, std::size_t vramBankCount, std::size_t eramBankCount, std::size_t wramBankCount) noexcept;

    // Sets all memory regions to 0
    constexpr void Reset() noexcept;

    constexpr bool IsInitialized() { return _isInitialized; }

    // Access a byte at a specific address, the stored result is only valid if it is marked successful.
    // ResultAccessInvalidBank is always a failure case.
    // ResultAccessInvalidAddress is always a failure case.
    // ResultAccessProhibitedAddress is sometimes returned as a failure case.
    // ResultAccessReadOnlyProhibitedAddress is never a failure case.
    constexpr AccessResultSet AccessByte(const MemoryAddress&) const noexcept;

    // Write a byte at a specific address if it is accessible and returns the const reference to the value.
    // If the address is not accessible, the function will propagate the AccessByte error.
    // If access returns ResultAccessReadOnlyProhibitedAddress, the result of this function is a failure and the read value is in the result.
    // Note that this function will write as long as the address is within a valid range and the address is not ReadOnlyProhibited.
    constexpr AccessResultSet WriteByte(const MemoryAddress&, const Byte& value) noexcept;
};

namespace
{
template <std::size_t N>
constexpr static bool IsValidBankCount(const std::size_t (&range)[N], std::size_t value)
{
    return !(std::find(std::begin(range), std::end(range), value) == std::end(range));
}

} // namespace

constexpr MemoryMap::InitializeResultSet MemoryMap::Initialize(const Byte (&rom)[], const std::size_t size) noexcept
{
    // TODO: Parse cartridge header
    InitializeResultSet result = Initialize(1, 1, 1, 1);
    if (result.IsFailure())
    {
        return result;
    }

    if ((size % RomBankSize) > 0)
    {
        return ResultInitializeInvalidAlignment(false);
    }

    std::size_t offset = 0;
    std::size_t bank   = 0;
    while (offset < size)
    {
        std::copy(&rom[offset], &rom[offset + RomBankSize], &_rom[bank++][0]);
        offset += RomBankSize;
    }

    return InitializeResultSet::DefaultResultSuccess();
}

constexpr MemoryMap::InitializeResultSet MemoryMap::Initialize(std::size_t romBankCount, std::size_t vramBankCount, std::size_t eramBankCount, std::size_t wramBankCount) noexcept
{
    if (_isInitialized)
    {
        return ResultInitializeAlreadyInitialized(false);
    }

    if (!IsValidBankCount(ValidRomBankCount, romBankCount))
    {
        return ResultInitializeInvalidRomBankCount(false);
    }

    if (!IsValidBankCount(ValidVramBankCount, vramBankCount))
    {
        // Either 1 in GB or 2 in CGB
        return ResultInitializeInvalidVramBankCount(false);
    }

    if (!IsValidBankCount(ValidExternalRamBankCount, eramBankCount))
    {
        return ResultInitializeInvalidEramBankCount(false);
    }

    if (!IsValidBankCount(ValidWramBankCount, wramBankCount))
    {
        // Either 2 in GB or 8 in CGB
        return ResultInitializeInvalidWramBankCount(false);
    }

    _romBankCount  = romBankCount;
    _vramBankCount = vramBankCount;
    _eramBankCount = eramBankCount;
    _wramBankCount = wramBankCount;

    _isInitialized = true;
    return InitializeResultSet::DefaultResultSuccess();
}

constexpr void MemoryMap::Reset() noexcept
{
    // Reset all memory regions
    for (auto& b : _rom)
    {
        b.Reset();
    }

    for (auto& b : _vram)
    {
        b.Reset();
    }

    for (auto& b : _eram)
    {
        b.Reset();
    }

    for (auto& b : _wram)
    {
        b.Reset();
    }

    _oam.Reset();
    _hram.Reset();
    _io.Reset();

    _isInitialized = false;
}

constexpr MemoryMap::AccessResultSet MemoryMap::AccessByte(const MemoryAddress& maddr) const noexcept
{
    auto bank    = maddr.bank;
    auto address = maddr.address;

    if (address <= 0x3FFF)
    {
        return AccessResultSet::DefaultResultSuccess(_rom[0][address]);
    }
    else if (address <= 0x7FFF)
    {
        return (bank >= _romBankCount) ? AccessResultSet(ResultAccessInvalidBank(false), 0) : AccessResultSet::DefaultResultSuccess(_rom[bank][address - 0x4000]);
    }
    else if (address <= 0x9FFF)
    {
        return (bank >= _vramBankCount) ? AccessResultSet(ResultAccessInvalidBank(false), 0) : AccessResultSet::DefaultResultSuccess(_vram[bank][address - 0x8000]);
    }
    else if (address <= 0xBFFF)
    {
        return bank >= _eramBankCount ? AccessResultSet(ResultAccessInvalidBank(false), 0) : AccessResultSet::DefaultResultSuccess(_eram[bank][address - 0xA000]);
    }
    else if (address <= 0xCFFF)
    {
        return AccessResultSet::DefaultResultSuccess(_wram[0][address - 0xC000]);
    }
    else if (address <= 0xDFFF)
    {
        return bank >= _wramBankCount ? AccessResultSet(ResultAccessInvalidBank(false), 0) : AccessResultSet::DefaultResultSuccess(_wram[bank][address - 0xD000]);
    }
    else if (address <= 0xFDFF)
    {
        // Echo RAM
        // From pandocs: "The range E000-FDFF is mapped to WRAM, but only the lower 13 bits of the address lines are connected"
        // Return a prohibited address warning, but otherwise the value is still returned
        address -= 0x2000;
        if (bank >= _wramBankCount)
        {
            return AccessResultSet(ResultAccessInvalidBank(false), 0);
        }

        auto value = AccessByte({bank, address});
        if (value.IsSuccess())
        {
            return AccessResultSet(ResultAccessProhibitedAddress(true), value);
        }
        else
        {
            return AccessResultSet(ResultAccessProhibitedAddress(false), value);
        }
    }
    else if (address <= 0xFE9F)
    {
        return AccessResultSet::DefaultResultSuccess(_oam[address - 0xFE00]);
    }
    else if (address <= 0xFEFF)
    {
        // From pandocs: "Use of this area is prohibited. This area returns $FF when OAM is blocked, and otherwise the behavior depends on the hardware revision."
        // On DMG, MGB, SGB, and SGB2, reads during OAM block trigger OAM corruption. Reads otherwise return $00.
        // On CGB revisions 0-D, this area is a unique RAM area, but is masked with a revision-specific value.
        // On CGB revision E, AGB, AGS, and GBP, it returns the high nibble of the lower address byte twice, e.g. FFAx returns $AA, FFBx returns $BB, and so forth.

        // For ease of implementation, just use CGB Revision E that returns the high nibble of the lower address byte twice
        // TODO: This area should be treated similarly to IO registers with a special handler that relies on OAM state
        return AccessResultSet(ResultAccessReadOnlyProhibitedAddress(true), _FEA0_FEFF[((address & 0x00F0) >> 4) - 0xA]);
    }
    else if (address <= 0xFF7F)
    {
        // IO registers are controlled via the CPU layer, but all have some memory representation underneath
        return AccessResultSet::DefaultResultSuccess(_io[address - 0xFF00]);
    }
    else if (address <= 0xFFFE)
    {
        return AccessResultSet::DefaultResultSuccess(_hram[address - 0xFF80]);
    }
    else if (address == 0xFFFF)
    {
        return AccessResultSet::DefaultResultSuccess(_registers.IE());
    }
    else
    {
        return AccessResultSet(ResultAccessInvalidAddress(false), 0);
    }
}

constexpr MemoryMap::AccessResultSet MemoryMap::WriteByte(const MemoryAddress& maddr, const Byte& value) noexcept
{
    AccessResultSet accessResult = AccessByte(maddr);
    if (accessResult.IsFailure())
    {
        return accessResult;
    }

    if (accessResult.IsResult<ResultAccessReadOnlyProhibitedAddress>())
    {
        return AccessResultSet(ResultAccessReadOnlyProhibitedAddress(false), static_cast<const Byte&>(accessResult));
    }

    auto& valueRef = const_cast<Byte&>(static_cast<const Byte&>(accessResult));
    valueRef       = value;

    // Make sure we propagate prohibited access issues up
    if (accessResult.IsResult<ResultAccessProhibitedAddress>())
    {
        return AccessResultSet(ResultAccessProhibitedAddress(true), valueRef);
    }
    else
    {
        return AccessResultSet::DefaultResultSuccess(valueRef);
    }
}

} // namespace pgb::memory