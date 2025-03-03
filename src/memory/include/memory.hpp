#pragma once

#include <bit>
#include <cstdint>

#include <common/block.hpp>
#include <common/datatypes.hpp>
#include <common/result.hpp>

namespace pgb::memory
{

namespace
{
constexpr static std::size_t ByteAccessWidth = 0x8;

//
constexpr static std::size_t MaxBankValue    = 0x1FF;
constexpr static std::size_t MaxAddressValue = 0xFFFF;

} // namespace

// Function results
using ResultAccessInvalidBank       = common::Result<"Bank not in valid range">;
using ResultAccessInvalidAddress    = common::Result<"Address not in valid range">;
using ResultAccessProhibitedAddress = common::Result<"Accessing prohibited address">;

using namespace pgb::common::block;
using namespace pgb::common::datatypes;

// GB has a 16-bit address space to map IO, ROM, & RAM
// This class provides an interface to the raw  underlying memory representation
class MemoryMap
{
private:
    // TODO: For now, just leave it as static/const
    static const std::size_t _romBankCount  = 0x200;
    static const std::size_t _vramBankCount = 2;
    static const std::size_t _eramBankCount = 2;
    static const std::size_t _wramBankCount = 8;

    // Actual memory regions
    Block<0x4000 * ByteAccessWidth, ByteAccessWidth> _rom[_romBankCount];
    Block<0x2000 * ByteAccessWidth, ByteAccessWidth> _vram[_vramBankCount];
    Block<0x2000 * ByteAccessWidth, ByteAccessWidth> _eram[_eramBankCount];
    Block<0x1000 * ByteAccessWidth, ByteAccessWidth> _wram[_wramBankCount];
    Block<0xA0 * ByteAccessWidth, ByteAccessWidth>   _oam;
    Block<0x80 * ByteAccessWidth, ByteAccessWidth>   _hram;
    // IO Registers & the interrupt enable register should be handled as individual registers
    Block<0x80 * ByteAccessWidth, ByteAccessWidth> _io;
    // // Interrupt enable uses 5 bits, so we map it to one byte
    Block<ByteAccessWidth, ByteAccessWidth> _ie;

public:
    struct MemoryAddress
    {
        uint_fast16_t bank : std::bit_width(static_cast<uint_fast16_t>(MaxBankValue));
        uint_fast16_t address : std::bit_width(static_cast<uint_fast16_t>(MaxAddressValue));
    };
    using AccessResultSet = common::ResultSet<const Byte&, common::ResultSuccess, ResultAccessInvalidBank, ResultAccessInvalidAddress, ResultAccessProhibitedAddress>;
    // Access a byte at a specific address, the stored result is only valid if it is marked successful
    AccessResultSet AccessByte(const MemoryAddress&) const noexcept;

    // Write a byte at a specific address if we are able to access it and returns the value written, otherwise will propagate the AccessByte error
    // Note that this function will write regardless of conceptual restrictions, it's not the job of this class to prevent access
    AccessResultSet WriteByte(const MemoryAddress&, const Byte& value) noexcept;
};

} // namespace pgb::memory