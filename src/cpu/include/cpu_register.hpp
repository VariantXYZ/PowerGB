#pragma once

#include <cstddef>

#include <common/util.hpp>

namespace pgb::cpu
{

template <bool EnableReadHi, bool EnableWriteHi, bool EnableReadLo, bool EnableWriteLo>
class GBCCPURegister
{
private:
    constexpr static unsigned char LowByte  = util::is_little_endian<uint16_t>() ? 0 : 1;
    constexpr static unsigned char HighByte = util::is_little_endian<uint16_t>() ? 1 : 0;

    std::uint16_t reg16;
    std::uint8_t(&reg8)[2] = reinterpret_cast<std::uint8_t (&)[2]>(reg16);

public:
    GBCCPURegister() { reg16 = 0; }

    static constexpr std::uint8_t LowNibble(uint8_t value)
    {
        return value & 0b00001111;
    }

    static constexpr std::uint8_t HighNibble(uint8_t value)
    {
        return value & 0b11110000;
    }

    [[nodiscard]] constexpr operator std::uint16_t() const
    {
        return reg16;
    }

    constexpr operator std::uint16_t&()
    {
        return reg16;
    }

    [[nodiscard]] constexpr std::uint8_t Hi() const
        requires EnableReadHi
    {
        return reg8[HighByte];
    }

    constexpr std::uint8_t& Hi()
        requires EnableWriteHi
    {
        return reg8[HighByte];
    }

    [[nodiscard]] constexpr std::uint8_t Lo() const
        requires EnableReadLo
    {
        return reg8[LowByte];
    }

    constexpr std::uint8_t& Lo()
        requires EnableWriteLo
    {
        return reg8[LowByte];
    }
};

} // namespace pgb::cpu