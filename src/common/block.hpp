#pragma once

#include <cstddef>

#include <common/datatypes.hpp>
#include <common/util.hpp>

namespace pgb::common::block
{

template <std::size_t BlockWidth>
concept IsValidBlockWidth = BlockWidth >= 8 && (BlockWidth % 8 == 0);

template <std::size_t AccessWidth>
concept IsValidAccessWidth = AccessWidth == 16 || AccessWidth == 8 || AccessWidth == 4;

// Some registers (IO, flags) will heavily use nibbles, whereas others will just operate as 8 or 16-bit data.
// This class aims to provide a register reprsentation that handles arbitrary lengths and access priorities, while maintaining portability.
// It explicitly avoids use of functions and features that are not mandated on every implementation (this includes certain fixed-width types).
template <std::size_t BlockWidth, std::size_t AccessWidth>
    requires IsValidBlockWidth<BlockWidth> && IsValidAccessWidth<AccessWidth>
class Block
{
private:
    static_assert(BlockWidth >= AccessWidth, "AccessWidth cannot be higher than the BlockWidth");
    static_assert(BlockWidth % AccessWidth == 0, "BlockWidth must be a multiple of the AccessWidth");
    static constexpr std::size_t ElementCount = BlockWidth / AccessWidth;
    using AccessType                          = std::conditional_t<AccessWidth == 4, datatypes::Nibble, typename std::conditional_t<AccessWidth == 8, datatypes::Byte, datatypes::Word>>;
    AccessType _register[ElementCount]{};

public:
    constexpr static std::size_t Size() noexcept { return ElementCount; }
    constexpr static std::size_t Granularity() noexcept { return AccessWidth; }

    constexpr void Reset()
    {
        std::fill_n(_register, ElementCount, 0);
    }

    // Provide direct data if the access width is the same
    template <std::size_t N>
    [[nodiscard]] constexpr AccessType Self() const noexcept
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    template <std::size_t N>
    constexpr AccessType& Self() noexcept
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    constexpr AccessType& operator[](int n) noexcept
    {
        return _register[n];
    }

    [[nodiscard]] constexpr const AccessType& operator[](int n) const noexcept
    {
        return _register[n];
    }

    template <std::size_t N>
    [[nodiscard]] constexpr datatypes::Nibble Nibble() const noexcept
        requires(std::is_same_v<AccessType, datatypes::Nibble>)
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    template <std::size_t N>
    constexpr datatypes::Nibble& Nibble() noexcept
        requires(std::is_same_v<AccessType, datatypes::Nibble>)
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    template <std::size_t N>
    [[nodiscard]] constexpr datatypes::Byte Byte() const noexcept
        requires(std::is_same_v<AccessType, datatypes::Byte>)
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    template <std::size_t N>
    datatypes::Byte& Byte() noexcept
        requires(std::is_same_v<AccessType, datatypes::Byte>)
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    template <std::size_t N>
    [[nodiscard]] constexpr datatypes::Word Word() const noexcept
        requires(std::is_same_v<AccessType, datatypes::Word>)
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    template <std::size_t N>
    datatypes::Word& Word() noexcept
        requires(std::is_same_v<AccessType, datatypes::Word>)
    {
        static_assert(N < ElementCount);
        return _register[N];
    }

    // For when access priority doesn't match width, we must do some work
    // Since we only care about 3 widths (4, 8, 16), we will just define them explicitly

    //// Nibble operations
    template <std::size_t N>
    [[nodiscard]] constexpr datatypes::Nibble Nibble() const noexcept
        requires(AccessWidth > 4)
    {
        static_assert(N < ElementCount * (AccessWidth / datatypes::Nibble::TypeWidth));
        constexpr std::size_t PerAccessType = (AccessWidth / datatypes::Nibble::TypeWidth);
        constexpr std::size_t Index         = N / PerAccessType;
        constexpr std::size_t SubIndex      = N % PerAccessType;
        return datatypes::Nibble(static_cast<datatypes::Nibble::Type>((_register[Index] >> ((PerAccessType - SubIndex - 1) * datatypes::Nibble::TypeWidth)) & datatypes::Nibble::MaxValue));
    }

    template <std::size_t N>
    constexpr void Nibble(const datatypes::Nibble& value) noexcept
    {
        static_assert(N < ElementCount * (AccessWidth / datatypes::Nibble::TypeWidth));

        if constexpr (AccessWidth == 4)
        {
            _register[N] = value;
        }
        else
        {
            constexpr std::size_t PerAccessType = (AccessWidth / datatypes::Nibble::TypeWidth);
            constexpr std::size_t Index         = N / PerAccessType;
            constexpr std::size_t SubIndex      = N % PerAccessType;
            std::size_t           Mask          = static_cast<std::size_t>(value.data) << ((PerAccessType - SubIndex - 1) * datatypes::Nibble::TypeWidth);

            _register[Index].data &= ~(0xF << ((PerAccessType - SubIndex - 1) * datatypes::Nibble::TypeWidth));
            _register[Index].data |= Mask;
        }
    }

    //// Byte operations
    template <std::size_t N>
    [[nodiscard]] constexpr datatypes::Byte Byte() const noexcept
        requires(BlockWidth >= 8) && (AccessWidth == 4 || AccessWidth == 16)
    {
        if constexpr (AccessWidth == 4)
        {
            static_assert(N < ElementCount / (datatypes::Byte::TypeWidth / AccessWidth));
            constexpr std::size_t StartIndex = (N * (datatypes::Byte::TypeWidth / AccessWidth));

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
    constexpr void Byte(const datatypes::Nibble& high, const datatypes::Nibble& low) noexcept
        requires(BlockWidth >= 8) && (AccessWidth == 4 || AccessWidth == 16)
    {
        if constexpr (AccessWidth == 4)
        {
            static_assert(N < ElementCount / (datatypes::Byte::TypeWidth / AccessWidth));
            constexpr std::size_t StartIndex = (N * (datatypes::Byte::TypeWidth / AccessWidth));
            _register[StartIndex + 0]        = high;
            _register[StartIndex + 1]        = low;
        }
        else if constexpr (AccessWidth == 8)
        {
            static_assert(N < ElementCount);
            _register[N].HighNibble(high);
            _register[N].LowNibble(low);
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
    constexpr void Byte(const datatypes::Byte& value) noexcept
        requires(BlockWidth >= 8)
    {
        if constexpr (AccessWidth == 4)
        {
            static_assert(N < ElementCount / (datatypes::Byte::TypeWidth / AccessWidth));
            constexpr std::size_t StartIndex = (N * (datatypes::Byte::TypeWidth / AccessWidth));
            _register[StartIndex + 0]        = value.HighNibble();
            _register[StartIndex + 1]        = value.LowNibble();
        }
        else if constexpr (AccessWidth == 8)
        {
            static_assert(N < ElementCount);
            _register[N] = value;
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
    template <std::size_t N>
    [[nodiscard]] constexpr datatypes::Word Word() const noexcept
        requires(BlockWidth >= 16) && (AccessWidth == 4 || AccessWidth == 8)
    {
        static_assert(N < ElementCount / (datatypes::Word::TypeWidth / AccessWidth));
        constexpr std::size_t StartIndex = (N * (datatypes::Word::TypeWidth / AccessWidth));

        if constexpr (AccessWidth == 4)
        {
            return datatypes::Word(
                datatypes::Byte{_register[StartIndex + 0], _register[StartIndex + 1]},
                datatypes::Byte{_register[StartIndex + 2], _register[StartIndex + 3]}
            );
        }
        else if constexpr (AccessWidth == 8)
        {
            return datatypes::Word(
                _register[StartIndex + 0],
                _register[StartIndex + 1]
            );
        }
    }

    template <std::size_t N>
    constexpr void Word(const datatypes::Byte& high, const datatypes::Byte& low) noexcept
        requires(BlockWidth >= 16)
    {
        static_assert(N < ElementCount / (datatypes::Word::TypeWidth / AccessWidth));
        constexpr std::size_t StartIndex = (N * (datatypes::Word::TypeWidth / AccessWidth));

        if constexpr (AccessWidth == 4)
        {
            _register[StartIndex + 0] = high.HighNibble();
            _register[StartIndex + 1] = high.LowNibble();
            _register[StartIndex + 2] = low.HighNibble();
            _register[StartIndex + 3] = low.LowNibble();
        }
        else if constexpr (AccessWidth == 8)
        {
            _register[StartIndex + 0] = high;
            _register[StartIndex + 1] = low;
        }
        else if constexpr (AccessWidth == 16)
        {
            _register[StartIndex].HighByte(high);
            _register[StartIndex].LowByte(low);
        }
    }

    template <std::size_t N>
    constexpr void Word(const datatypes::Word& value) noexcept
        requires(BlockWidth >= 16)
    {
        static_assert(N < ElementCount / (datatypes::Word::TypeWidth / AccessWidth));
        constexpr std::size_t StartIndex = (N * (datatypes::Word::TypeWidth / AccessWidth));

        if constexpr (AccessWidth == 4)
        {
            _register[StartIndex + 0] = value.HighByte().HighNibble();
            _register[StartIndex + 1] = value.HighByte().LowNibble();
            _register[StartIndex + 2] = value.LowByte().HighNibble();
            _register[StartIndex + 3] = value.LowByte().LowNibble();
        }
        else if constexpr (AccessWidth == 8)
        {
            _register[StartIndex + 0] = value.HighByte();
            _register[StartIndex + 1] = value.LowByte();
        }
        else if constexpr (AccessWidth == 16)
        {
            _register[StartIndex] = value;
        }
    }
};

} // namespace pgb::common::block