#pragma once

#include <cstddef>
#include <type_traits>

#include <common/util.hpp>

// A utility class definition that provides a compile-time append-able registry of types within the same TU
// Intended to be used to allow for grouping various template instantiations within the same TU (e.g., define operations in different headers and automatically have them registered)
// Takes advantage of friend-injection and some other compile-time tricks

namespace pgb::common::registry
{

// List object that stores all types
template <class... Ts>
    requires(sizeof...(Ts) == 0 || util::types_are_unique<Ts...>)
struct List
{
    constexpr static std::size_t Size = sizeof...(Ts);
};

namespace detail
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wnon-template-friend"

// Forward-declaration of Nth, used as a unique key in the list (Tag + 'N')
// 'Tag' specifies a particular registry
template <typename Tag, auto>
struct Nth
{
    // Friend-injection: Forward-declare friend function (defined via ADL in 'Set')
    auto friend Get(Nth) noexcept;
};

#pragma GCC diagnostic pop

// For a particular Tag + N, set 'T'
// This effectively 'stores' a mapping
template <typename Tag, auto N, class T>
struct Set
{
    // 'Get' is defined to return T, used in Append later to allow for "updating" List with a new type'
    auto friend Get(Nth<Tag, N>) noexcept { return T{}; }
};

// Create a new type list based on an old one and a new type added to the end
template <class T, template <class...> class TList, class... Ts>
auto Append(TList<Ts...>) noexcept -> TList<Ts..., T>{};

} // namespace detail

// Search for the first open slot (Tag + N, fixed Tag), recursively calling itself
// Uses the existence of Get to see if the Tag + N is accounted for
// Either create a new list if nothing else exists, or eventually call detail::Append to update the type list
template <class T, typename Tag, auto N = 0>
constexpr auto Append() noexcept
{
    if constexpr (requires { Get(detail::Nth<Tag, N>{}); })
    {
        // As we go down the list, make sure this type isn't already registered'
        static_assert(!std::is_same<T, decltype(Get(detail::Nth<Tag, N>{}))>::value, "Type is already registered");
        Append<T, Tag, N + 1>();
    }
    else if constexpr (N == 0)
    {
        void(detail::Set<Tag, N, List<T>>{});
    }
    else
    {
        void(detail::Set<Tag, N, decltype(detail::Append<T>(Get(detail::Nth<Tag, N - 1>{})))>{});
    }
    return true;
}

// Return the underlying compile-time List for a given Tag
// Will recursively search until an empty slot is found and return the N-1 entry
template <typename Tag, auto N = 0>
constexpr auto Registry() noexcept
{
    if constexpr (requires { Get(detail::Nth<Tag, N>{}); })
    {
        return Registry<Tag, N + 1>();
    }
    else if constexpr (N == 0)
    {
        return List{};
    }
    else
    {
        return Get(detail::Nth<Tag, N - 1>{});
    }
}

} // namespace pgb::common::registry
