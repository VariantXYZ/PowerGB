#pragma once

#include <bit>
#include <cstddef>
#include <type_traits>

namespace pgb::common::util
{

template <typename T>
constexpr static bool is_little_endian()
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

} // namespace pgb::common::util