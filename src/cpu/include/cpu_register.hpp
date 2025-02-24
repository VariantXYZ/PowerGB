#pragma once

#include <bit>
#include <cstdint>

namespace pgb::cpu
{

template <bool EnableReadHi, bool EnableWriteHi, bool EnableReadLo, bool EnableWriteLo>
class GBCCPURegister
{
private:
    constexpr static size_t LowBit  = (std::endian::native == std::endian::little) ? 0 : 1;
    constexpr static size_t HighBit = (std::endian::native == std::endian::little) ? 1 : 0;

    std::uint8_t   reg8[2];
    std::uint16_t& reg16 = reinterpret_cast<std::uint16_t&>(*reg8);

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
        return reg8[HighBit];
    }

    constexpr std::uint8_t& Hi()
        requires EnableWriteHi
    {
        return reg8[HighBit];
    }

    [[nodiscard]] constexpr std::uint8_t Lo() const
        requires EnableReadLo
    {
        return reg8[LowBit];
    }

    constexpr std::uint8_t& Lo()
        requires EnableWriteLo
    {
        return reg8[LowBit];
    }
};

} // namespace pgb::cpu