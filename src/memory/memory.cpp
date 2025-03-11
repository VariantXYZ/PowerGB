#include <memory/include/memory.hpp>

#include <common/datatypes.hpp>

namespace pgb::memory
{

using namespace pgb::common::datatypes;

namespace
{
template <std::size_t N>
constexpr static bool IsValidBankCount(const std::size_t (&range)[N], std::size_t value)
{
    return !(std::find(std::begin(range), std::end(range), value) == std::end(range));
}

} // namespace

MemoryMap::InitializeResultSet MemoryMap::Initialize(const Byte (&rom)[], const std::size_t size) noexcept
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

MemoryMap::InitializeResultSet MemoryMap::Initialize(std::size_t romBankCount, std::size_t vramBankCount, std::size_t eramBankCount, std::size_t wramBankCount) noexcept
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

void MemoryMap::Reset() noexcept
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

MemoryMap::AccessResultSet MemoryMap::AccessByte(const MemoryAddress& maddr) const noexcept
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
        static const Byte _FEA0_FEFF[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
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

MemoryMap::AccessResultSet MemoryMap::WriteByte(const MemoryAddress& maddr, const Byte& value) noexcept
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