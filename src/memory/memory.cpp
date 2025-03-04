#include <memory/include/memory.hpp>

#include <common/datatypes.hpp>

namespace pgb::memory
{

using namespace pgb::common::datatypes;

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

    if (std::find(std::begin(ValidRomBankCount), std::end(ValidRomBankCount), romBankCount) == std::end(ValidRomBankCount))
    {
        return ResultInitializeInvalidRomBankCount(false);
    }

    if (vramBankCount != 1 && vramBankCount != 2)
    {
        // Either 1 in GB or 2 in CGB
        return ResultInitializeInvalidVramBankCount(false);
    }

    if (std::find(std::begin(ValidExternalRamBankCount), std::end(ValidExternalRamBankCount), eramBankCount) == std::end(ValidExternalRamBankCount))
    {
        return ResultInitializeInvalidEramBankCount(false);
    }

    if (wramBankCount != 2 && wramBankCount != 8)
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
        address = address & 0b0001111111111111;
        return bank >= _wramBankCount ? AccessResultSet(ResultAccessInvalidBank(false), 0) : AccessResultSet(ResultAccessProhibitedAddress(true), _wram[bank][address - 0xC000]);
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
        return AccessResultSet::DefaultResultSuccess(_cpu.IE());
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

    auto&      valueRef = const_cast<Byte&>(static_cast<const Byte&>(accessResult));
    const Byte oldValue = valueRef;
    valueRef            = value;

    // Make sure we propagate prohibited access issues up
    if (accessResult.IsResult<ResultAccessProhibitedAddress>())
    {
        return AccessResultSet(ResultAccessProhibitedAddress(true), oldValue);
    }
    else
    {
        return AccessResultSet::DefaultResultSuccess(oldValue);
    }
}

} // namespace pgb::memory