#include <memory/include/memory.hpp>

#include <common/datatypes.hpp>

namespace pgb::memory
{

using namespace pgb::common::datatypes;

MemoryMap::AccessResultSet MemoryMap::AccessByte(const MemoryAddress& maddr) const noexcept
{
    auto bank    = maddr.bank;
    auto address = maddr.address;

    if (address <= 0x7FFF)
    {
        return (bank >= _romBankCount) ? AccessResultSet(ResultAccessInvalidBank(false), bank) : AccessResultSet::DefaultResultSuccess(_rom[bank][address]);
    }
    else if (address <= 0x9FFF)
    {
        return (bank >= _vramBankCount) ? AccessResultSet(ResultAccessInvalidBank(false), bank) : AccessResultSet::DefaultResultSuccess(_vram[bank][address - 0x8000]);
    }
    else if (address <= 0xBFFF)
    {
        return bank >= _eramBankCount ? AccessResultSet(ResultAccessInvalidBank(false), bank) : AccessResultSet::DefaultResultSuccess(_eram[bank][address - 0xA000]);
    }
    else if (address <= 0xDFFF)
    {
        return bank >= _wramBankCount ? AccessResultSet(ResultAccessInvalidBank(false), bank) : AccessResultSet::DefaultResultSuccess(_wram[bank][address - 0xC000]);
    }
    else if (address <= 0xFDFF)
    {
        // Echo RAM
        // From pandocs: "The range E000-FDFF is mapped to WRAM, but only the lower 13 bits of the address lines are connected"
        // Return a prohibited address warning, but otherwise the value is still returned
        address = address & 0b0001111111111111;
        return bank >= _wramBankCount ? AccessResultSet(ResultAccessInvalidBank(false), bank) : AccessResultSet(ResultAccessProhibitedAddress(true), _wram[bank][address - 0xC000]);
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
        return AccessResultSet(ResultAccessProhibitedAddress(true), _FEA0_FEFF[((address & 0x00F0) >> 4) - 0xA]);
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
        return AccessResultSet::DefaultResultSuccess(_ie[0]);
    }
    else
    {
        return AccessResultSet(ResultAccessInvalidAddress(false), address);
    }
}

MemoryMap::AccessResultSet MemoryMap::WriteByte(const MemoryAddress& maddr, const Byte& value) noexcept
{
    AccessResultSet accessResult = AccessByte(maddr);

    if (accessResult.IsFailure())
    {
        return accessResult;
    }

    // Make sure we propagate prohibited access issues up
    const_cast<Byte&>(static_cast<const Byte&>(accessResult)) = value;

    if (accessResult.IsResult<ResultAccessProhibitedAddress>())
    {
        return AccessResultSet(ResultAccessProhibitedAddress(true), value);
    }
    else
    {
        return AccessResultSet::DefaultResultSuccess(value);
    }
}

} // namespace pgb::memory