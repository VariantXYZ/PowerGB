#pragma once

#include <cstddef>
#include <cstdint>

// Common accessed datatypes
// The ultimate goal of this is portability
// Regardless of platform register width, endianness or type sizes, a fixed representation is provided
namespace pgb::common::datatypes
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

    [[nodiscard]] constexpr operator Type() const noexcept { return data; }

    constexpr Datatype& operator++() noexcept
    {
        ++data;
        return *this;
    }

    constexpr Datatype& operator--() noexcept
    {
        --data;
        return *this;
    }

    constexpr Datatype operator++(int) noexcept
    {
        auto tmp = *this;
        data++;
        return tmp;
    }

    constexpr Datatype operator--(int) noexcept
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

    constexpr Byte(const Nibble& high, const Nibble& low) noexcept
    {
        data = 0;
        HighNibble(high);
        LowNibble(low);
    }

    [[nodiscard]] constexpr Nibble HighNibble() const noexcept
    {
        struct Nibble n{static_cast<Nibble::Type>((data & 0xF0) >> (TypeWidth / 2))};
        return n;
    }

    [[nodiscard]] constexpr Nibble LowNibble() const noexcept
    {
        struct Nibble n{static_cast<Nibble::Type>(data & 0x0F)};
        return n;
    }

    constexpr void HighNibble(const Nibble& value) noexcept
    {
        data = (data & 0x0F) | (value.data << (TypeWidth / 2));
    }

    constexpr void LowNibble(const Nibble& value) noexcept
    {
        data = (data & 0xF0) | (value.data);
    }
};

struct Word : Datatype<std::uint_fast16_t, 16, 0, 0xFFFF>
{
    using Datatype::Datatype;

    constexpr Word(Byte high, Byte low) noexcept
    {
        data = 0;
        HighByte(high);
        LowByte(low);
    }

    [[nodiscard]] constexpr Byte HighByte() const noexcept
    {
        struct Byte b = {static_cast<const Byte::Type>((data & 0xFF00) >> (TypeWidth / 2))};
        return b;
    }

    [[nodiscard]] constexpr Byte LowByte() const noexcept
    {
        struct Byte b{static_cast<const Byte::Type>(data & 0x00FF)};
        return b;
    }

    constexpr void HighByte(const Byte& value) noexcept
    {
        data = (data & 0x00FF) | (value.data << (TypeWidth / 2));
    }

    constexpr void LowByte(const Byte& value) noexcept
    {
        data = (data & 0xFF00) | (value.data);
    }
};

} // namespace pgb::common::datatypes