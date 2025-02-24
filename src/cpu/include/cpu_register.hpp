#pragma once

#include <cstdint>

namespace pgb::cpu
{

template <bool EnableReadHi, bool EnableWriteHi, bool EnableReadLo, bool EnableWriteLo>
class GBCCPURegister
{
private:
    union
    {
        std::uint8_t  reg8[2];
        std::uint16_t reg16;
    } value;

public:
    GBCCPURegister() { value.reg16 = 0; }

    [[nodiscard]] constexpr operator std::uint16_t() const
    {
        return value.reg16;
    }

    constexpr operator std::uint16_t&()
    {
        return value.reg16;
    }

    [[nodiscard]] constexpr std::uint8_t Hi() const
        requires EnableReadHi
    {
        return value.reg8[1];
    }

    constexpr std::uint8_t& Hi()
        requires EnableWriteHi
    {
        return value.reg8[1];
    }

    [[nodiscard]] constexpr std::uint8_t Lo() const
        requires EnableReadHi
    {
        return value.reg8[0];
    }

    constexpr std::uint8_t& Lo()
        requires EnableWriteLo
    {
        return value.reg8[0];
    }
};

} // namespace pgb::cpu