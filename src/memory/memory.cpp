#include "common/result.hpp"
#include <memory/include/memory.hpp>

#include <common/datatypes.hpp>

#include <iostream>
#include <utility>

namespace pgb::memory
{

using namespace pgb::common::datatypes;

const Byte& MemoryMap::AccessByte(std::size_t bank, std::size_t address) const
{
    if (address <= 0x7FFF)
    {
        return _rom[bank][address];
    }
    else if (address <= 0x9FFF)
    {
        return _vram[bank][address - 0x7FFF];
    }
    else if (address <= 0xBFFF)
    {
        return _eram[bank][address - 0x9FFF];
    }
    else if (address <= 0xDFFF)
    {
        return _wram[bank][address - 0xBFFF];
    }
    else if (address <= 0xFDFF)
    {
        // Echo RAM
        // "The range E000-FDFF is mapped to WRAM, but only the lower 13 bits of the address lines are connected"
        return AccessByte(bank, address & 0b0001111111111111);
    }
    else if (address <= 0xFE9F)
    {
        return _oam[address - 0xFDFF];
    }
    else if (address <= 0xFEFF)
    {
        // TODO: Unused region
        return _rom[0][0];
    }
    else if (address <= 0xFF7F)
    {
        // TODO: IO registers
        return _rom[0][0];
    }
    else if (address <= 0xFFFE)
    {
        return _hram[address - 0xFF7F];
    }
    else if (address == 0xFFFF)
    {
        return _ie[0];
    }
    else
    {
        // TODO: Error case
        return _rom[0][0];
    }
}

void MemoryMap::WriteByte(std::size_t bank, std::size_t address, const Byte& value)
{
    const_cast<Byte&>(AccessByte(bank, address)) = value;
}

} // namespace pgb::memory