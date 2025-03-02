#pragma once

#include <common/block.hpp>
#include <common/datatypes.hpp>
#include <common/result.hpp>

namespace pgb::memory
{

namespace
{
using namespace pgb::common::datatypes;

constexpr static std::size_t ByteAccessWidth = 0x8;

// TODO: For now, just use the max, can setup dynamic allocation if necessary later
constexpr static std::size_t ROMBankCount    = 0x200;
constexpr static std::size_t VRAMBankCount   = 2;
constexpr static std::size_t ERAMBankCount   = 2;
constexpr static std::size_t WRAMBankCount   = 8;

} // namespace

// Function results
using ResultAccessInvalidBank    = common::Result<"Bank not in valid range">;
using ResultAccessInvalidAddress = common::Result<"Address not in valid range">;

using namespace pgb::common::block;

// GB has a 16-bit address space to map IO, ROM, & RAM
// This class aims to act as a 'bus' to route memory accesses through
class MemoryMap
{
private:
    // Actual memory regions
    Block<0x4000 * ByteAccessWidth, ByteAccessWidth> _rom[ROMBankCount];
    Block<0x2000 * ByteAccessWidth, ByteAccessWidth> _vram[VRAMBankCount];
    Block<0x2000 * ByteAccessWidth, ByteAccessWidth> _eram[ERAMBankCount];
    Block<0x1000 * ByteAccessWidth, ByteAccessWidth> _wram[WRAMBankCount];
    Block<0x9F * ByteAccessWidth, ByteAccessWidth>   _oam;
    Block<0x7E * ByteAccessWidth, ByteAccessWidth>   _hram;

    // // Echo RAM and the 'no access zone' can be handled specially
    // // IO Registers & the interrupt enable register should be handled as individual registers

    // // Interrupt enable uses 5 bits, so we map it to one byte
    Block<ByteAccessWidth, ByteAccessWidth> _ie;

public:
    using AccessResultSet = common::ResultSet<const Byte, common::ResultSuccess, ResultAccessInvalidBank, ResultAccessInvalidAddress>;
    const Byte& AccessByte(std::size_t bank, std::size_t address) const;
    void        WriteByte(std::size_t bank, std::size_t address, const Byte& value);
};

} // namespace pgb::memory