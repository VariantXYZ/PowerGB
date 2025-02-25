#pragma once

#include <cstddef>
#include <cstdint>

// Common accessed datatypes

namespace pgb::cpu::datatypes
{

namespace
{
template <typename T, std::size_t TW, T MiV, T MaV>
struct Datatype
{
    static constexpr std::size_t TypeWidth = TW;
    using Type                             = T;
    static constexpr Type MinValue         = MiV;
    static constexpr Type MaxValue         = MaV;

    Type data : TypeWidth                  = MinValue;

    constexpr Datatype()                   = default;
    constexpr Datatype(Type value) : data(value) {};

    [[nodiscard]] operator Type() const { return data; }

    constexpr Datatype& operator++()
    {
        ++data;
        return *this;
    }

    constexpr Datatype& operator--()
    {
        --data;
        return *this;
    }

    constexpr Datatype operator++(int)
    {
        auto tmp = *this;
        data++;
        return tmp;
    }

    constexpr Datatype operator--(int)
    {
        auto tmp = *this;
        data--;
        return tmp;
    }
};
} // namespace

struct Nibble : Datatype<std::uint_fast8_t, 4, 0, 0xF>
{
    using Datatype::Datatype;
};

struct Byte : Datatype<std::uint_fast8_t, 8, 0, 0xFF>
{
    using Datatype::Datatype;

    constexpr Byte(const Nibble& high, const Nibble& low)
    {
        data = 0;
        HighNibble(high);
        LowNibble(low);
    }

    [[nodiscard]] constexpr Nibble HighNibble() const
    {
        struct Nibble n{static_cast<Nibble::Type>((data & 0xF0) >> (TypeWidth / 2))};
        return n;
    }

    [[nodiscard]] constexpr Nibble LowNibble() const
    {
        struct Nibble n{static_cast<Nibble::Type>(data & 0x0F)};
        return n;
    }

    constexpr void HighNibble(const Nibble& value)
    {
        data = (data & 0x0F) | (value.data << (TypeWidth / 2));
    }

    constexpr void LowNibble(const Nibble& value)
    {
        data = (data & 0xF0) | (value.data);
    }
};

struct Word : Datatype<std::uint_fast16_t, 16, 0, 0xFFFF>
{
    using Datatype::Datatype;

    constexpr Word(Byte high, Byte low)
    {
        data = 0;
        HighByte(high);
        LowByte(low);
    }

    [[nodiscard]] constexpr Byte HighByte() const
    {
        struct Byte b = {static_cast<const Byte::Type>((data & 0xFF00) >> (TypeWidth / 2))};
        return b;
    }

    [[nodiscard]] constexpr Byte LowByte() const
    {
        struct Byte b{static_cast<const Byte::Type>(data & 0x00FF)};
        return b;
    }

    constexpr void HighByte(const Byte& value)
    {
        data = (data & 0x00FF) | (value.data << (TypeWidth / 2));
    }

    constexpr void LowByte(const Byte& value)
    {
        data = (data & 0xFF00) | (value.data);
    }
};

} // namespace pgb::cpu::datatypes