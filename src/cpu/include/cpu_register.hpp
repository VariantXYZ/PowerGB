#pragma once

#include <cstdint>

namespace pgb::cpu
{

template <bool EnableReadHi,
          bool EnableWriteHi,
          bool EnableReadLo,
          bool EnableWriteLo>
class GBCPURegister16
{
private:
    union
    {
        std::uint8_t  reg8[2];
        std::uint16_t reg16;
    } value;

public:
    GBCPURegister16() { value.reg16 = 0; }

    [[nodiscard]] operator std::uint16_t() const
    {
        return value.reg16;
    }

    operator std::uint16_t&()
    {
        return value.reg16;
    }

    [[nodiscard]] std::uint8_t Hi() const
        requires EnableReadHi
    {
        return value.reg8[1];
    }

    std::uint8_t& Hi()
        requires EnableWriteHi
    {
        return value.reg8[1];
    }

    [[nodiscard]] std::uint8_t Lo() const
        requires EnableReadHi
    {
        return value.reg8[0];
    }

    std::uint8_t& Lo()
        requires EnableWriteLo
    {
        return value.reg8[0];
    }
};

} // namespace pgb::cpu