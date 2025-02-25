#pragma once

#include <cstddef>
#include <cstdint>

#include <common/util.hpp>
#include <cpu/include/datatypes.hpp>

namespace pgb::cpu
{

template <std::size_t RegisterWidth>
concept IsValidRegisterWidth = RegisterWidth == 16 || RegisterWidth == 8;

template <std::size_t AccessWidth>
concept IsValidAccessWidth = AccessWidth == 16 || AccessWidth == 8 || AccessWidth == 4;

// Some registers (IO, flags) will heavily use nibbles, whereas others will just operate as 8 or 16-bit data.
// This class aims to provide a register reprsentation that handles arbitrary lengths and access priorities, while maintaining portability.
// It explicitly avoids use of functions and features that are not mandated on every implementation (this includes certain fixed-width types).
template <std::size_t RegisterWidth, std::size_t AccessWidth>
    requires IsValidRegisterWidth<RegisterWidth> && IsValidAccessWidth<RegisterWidth>
class Register
{
private:
    static_assert(RegisterWidth >= AccessWidth, "AccessWidth cannot be higher than the RegisterWidth");
    static_assert(RegisterWidth % AccessWidth == 0, "RegisterWidth must be a multiple of the AccessWidth");
    static constexpr std::size_t ElementCount = RegisterWidth / AccessWidth;
    using AccessType                          = std::conditional_t<AccessWidth == 4, datatypes::Nibble, typename std::conditional_t<AccessWidth == 8, datatypes::Byte, datatypes::Word>>;
    AccessType _register[ElementCount]{};

public:
    // Provide direct data if the access width is the same
    template <std::size_t N>
    [[nodiscard]] constexpr AccessType Self() const
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    template <std::size_t N>
    constexpr AccessType& Self()
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    template <std::size_t N>
    [[nodiscard]] constexpr datatypes::Nibble Nibble() const
        requires(std::is_same_v<AccessType, datatypes::Nibble>)
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    template <std::size_t N>
    constexpr datatypes::Nibble& Nibble()
        requires(std::is_same_v<AccessType, datatypes::Nibble>)
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    template <std::size_t N>
    [[nodiscard]] constexpr datatypes::Byte Byte() const
        requires(std::is_same_v<AccessType, datatypes::Byte>)
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    template <std::size_t N>
    struct Byte& Byte()
        requires(std::is_same_v<AccessType, datatypes::Byte>)
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    template <std::size_t N>
    [[nodiscard]] constexpr datatypes::Word Word() const
        requires(std::is_same_v<AccessType, datatypes::Word>)
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    template <std::size_t N>
    struct Word& Byte()
        requires(std::is_same_v<AccessType, datatypes::Word>)
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    // For when access priority doesn't match width, we must do some work
    // Since we only care about 3 widths (4, 8, 16), we will just define them explicitly

    //// Nibble operations
    template <std::size_t N>
    [[nodiscard]] constexpr datatypes::Nibble Nibble() const
        requires(AccessWidth > 4)
    {
        static_assert(N < ElementCount * (AccessWidth / datatypes::Nibble::TypeWidth));
        constexpr std::size_t       PerAccessType = (AccessWidth / datatypes::Nibble::TypeWidth);
        constexpr std::size_t       Index         = N / PerAccessType;
        constexpr std::size_t       SubIndex      = N % PerAccessType;
        constexpr std::uint_fast8_t value         = (_register[Index] >> ((PerAccessType - SubIndex) * datatypes::Nibble::TypeWidth)) & 0x0F;
        return Nibble(value);
    }

    template <std::size_t N>
    void Nibble(const datatypes::Nibble& value)
        requires(AccessWidth > 4)
    {
        static_assert(N < ElementCount * (AccessWidth / datatypes::Nibble::TypeWidth));
        constexpr std::size_t PerAccessType = (AccessWidth / datatypes::Nibble::TypeWidth);
        constexpr std::size_t Index         = N / PerAccessType;
        constexpr std::size_t SubIndex      = N % PerAccessType;
        constexpr std::size_t Mask          = (value.data << ((PerAccessType - SubIndex) * datatypes::Nibble::TypeWidth));

        _register[Index].data &= ~(0xFF << ((PerAccessType - SubIndex) * datatypes::Nibble::TypeWidth));
        _register[Index].data |= Mask;
    }

    //// Byte operations
    template <std::size_t N>
    [[nodiscard]] constexpr datatypes::Byte Byte() const
        requires(RegisterWidth >= 8) && (AccessWidth == 4 || AccessWidth == 16)
    {
        if constexpr (AccessWidth == 4)
        {
            static_assert(N < ElementCount / (datatypes::Byte::TypeWidth / AccessWidth));
            constexpr std::size_t StartIndex = (N * (datatypes::Byte::TypeWidth / AccessWidth));

            static_assert(AccessWidth == 4);
            return datatypes::Byte(
                _register[StartIndex + 0],
                _register[StartIndex + 1]
            );
        }
        else if constexpr (AccessWidth == 16)
        {
            static_assert(N < ElementCount * (AccessWidth / datatypes::Byte::TypeWidth));
            constexpr bool        IsHighByte = (N % 2) == 0;
            constexpr std::size_t Index      = N / (AccessWidth / datatypes::Byte::TypeWidth);

            return IsHighByte ? _register[Index].HighByte()
                              : _register[Index].LowByte();
        }
    }

    template <std::size_t N>
    constexpr void Byte(const datatypes::Nibble& high, const datatypes::Nibble& low)
        requires(RegisterWidth >= 8) && (AccessWidth == 4 || AccessWidth == 16)
    {
        if constexpr (AccessWidth == 4)
        {
            static_assert(N < ElementCount / (datatypes::Byte::TypeWidth / AccessWidth));
            constexpr std::size_t StartIndex = (N * (datatypes::Byte::TypeWidth / AccessWidth));
            _register[StartIndex + 0]        = high;
            _register[StartIndex + 1]        = low;
        }
        else if constexpr (AccessWidth == 16)
        {
            static_assert(N < ElementCount * (AccessWidth / datatypes::Byte::TypeWidth));
            constexpr bool        IsHighByte = (N % 2) == 0;
            constexpr std::size_t Index      = N / (AccessWidth / datatypes::Byte::TypeWidth);

            if constexpr (IsHighByte)
            {
                _register[Index].HighByte(Byte(high, low));
            }
            else
            {
                _register[Index].LowByte(Byte(high, low));
            }
        }
    }

    template <std::size_t N>
    constexpr void Byte(const datatypes::Byte& value)
        requires(RegisterWidth >= 8) && (AccessWidth == 4 || AccessWidth == 16)
    {
        if constexpr (AccessWidth == 4)
        {
            static_assert(N < ElementCount / (datatypes::Byte::TypeWidth / AccessWidth));
            constexpr std::size_t StartIndex = (N * (datatypes::Byte::TypeWidth / AccessWidth));
            _register[StartIndex + 0]        = value.HighNibble();
            _register[StartIndex + 1]        = value.LowNibble();
        }
        else if constexpr (AccessWidth == 16)
        {
            static_assert(N < ElementCount * (AccessWidth / datatypes::Byte::TypeWidth));
            constexpr bool        IsHighByte = (N % 2) == 0;
            constexpr std::size_t Index      = N / (AccessWidth / datatypes::Byte::TypeWidth);

            if constexpr (IsHighByte)
            {
                _register[Index].HighByte(value);
            }
            else
            {
                _register[Index].LowByte(value);
            }
        }
    }

    //// Word operations
};

} // namespace pgb::cpu