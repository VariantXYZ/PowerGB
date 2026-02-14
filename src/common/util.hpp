#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <type_traits>

namespace pgb::common::util
{

template <typename T>
constexpr static bool IsLittleEndian() noexcept
{
    static_assert(std::is_trivial_v<T>, "Type must be trivial");
    static_assert(sizeof(T) > 1, "Cannot determine endianness of type of size 1");
    struct Bytes
    {
        std::byte b[sizeof(T)];
    };
    T data = 1;
    return std::bit_cast<Bytes>(data).b[0] == std::byte{1};
}

template <std::size_t N>
struct StringLiteral
{
public:
    char                         value[N];
    static constexpr std::size_t Size = N;
    constexpr StringLiteral(const char (&str)[N]) noexcept
    {
        std::copy_n(str, N, value);
    }
};

template <typename T, typename... Types>
constexpr bool types_are_unique_v = (!std::is_same_v<T, Types> && ...) && types_are_unique_v<Types...>;
template <typename T>
constexpr bool types_are_unique_v<T> = true;

template <typename... T>
concept types_are_unique = (types_are_unique_v<T...>);

template <auto... Values>
consteval bool AllUniqueValues()
{
    constexpr std::array values{Values...};

    for (std::size_t i = 0; i < values.size(); ++i)
    {
        for (std::size_t j = i + 1; j < values.size(); ++j)
        {
            if (values[i] == values[j])
            {
                return false;
            }
        }
    }
    return true;
}

} // namespace pgb::common::util
