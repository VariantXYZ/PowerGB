#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>
#include <tuple>
#include <variant>

#include <common/util.hpp>

namespace pgb::common
{

// Result and Result set provide a result class that can:
// * Provide a text description without needing to know the underlying type
// * Allow the called function to define it as success or failure
// * Allow for a trivial integer-based comparison for result handling or explicit handling based on the type
// * Hold the function's useful return value if there is one
template <util::StringLiteral Description>
class Result
{
private:
    bool _isSuccess;

public:
    constexpr Result(bool isSuccess) noexcept : _isSuccess(isSuccess) {}
    static constexpr const char* GetDescription() noexcept { return Description.value; }

    constexpr bool IsSuccess() const noexcept { return _isSuccess; }
    constexpr bool IsFailure() const noexcept { return !_isSuccess; }
};

// A generic success and failure result
using ResultSuccess = Result<"Success">;
using ResultFailure = Result<"Failure">;

template <class R>
concept ResultType =
    requires(R r) {
        {
            Result{r}
        } -> std::same_as<R>;
    };

template <typename Type, ResultType... Results>
class ResultSet
{
    static_assert(sizeof...(Results) > 0, "There must be at least one result in a set.");

private:
    // Use of std::variant allows for use of 'holds_alternative'
    using ResultOptions = std::variant<Results...>;
    const ResultOptions _result;
    const Type          _value;

    using DefaultResultType                          = std::tuple_element_t<0, std::tuple<Results...>>;

    // To 'dynamically' access various Result types from std::variant via its index, we do two things:
    // * For descriptions, which are static methods, we can prepopulate the static array
    // * For Result member functions, have an ordered list of visitors based on type
    static constexpr const char* _descriptions[]     = {Results::GetDescription()...};

    using BoolVisitor                                = bool (*)(ResultOptions);
    static constexpr BoolVisitor _isSuccessVisitor[] = {[](ResultOptions v)
                                                        { return std::get<Results>(v).IsSuccess(); }...};
    static constexpr BoolVisitor _isFailureVisitor[] = {[](ResultOptions v)
                                                        { return std::get<Results>(v).IsFailure(); }...};

public:
    // Constructor will implicitly cast a result into the variant
    constexpr ResultSet(const ResultOptions& result, const Type& value) noexcept : _result(result), _value(value) {}

    constexpr operator Type() { return _value; }

    // Utility function for default result as success
    constexpr static ResultSet DefaultResultSuccess(const Type& value) noexcept
    {
        return ResultSet(DefaultResultType(true), value);
    }

    // Utility function for default result as failure
    constexpr static ResultSet DefaultResultFailure(const Type& value) noexcept
    {
        return ResultSet(DefaultResultType(false), value);
    }

    constexpr bool IsSuccess() const noexcept
    {
        static_assert(std::size(_isSuccessVisitor) == sizeof...(Results));
        return _isSuccessVisitor[_result.index()](_result);
    }

    constexpr bool IsFailure() const noexcept
    {
        static_assert(std::size(_isFailureVisitor) == sizeof...(Results));
        return _isFailureVisitor[_result.index()](_result);
    }

    constexpr const char* GetStatusDescription() const noexcept
    {
        static_assert(std::size(_descriptions) == sizeof...(Results));
        return _descriptions[_result.index()];
    }

    template <typename Rx>
    constexpr bool IsResult() const noexcept
        requires(std::is_same_v<Rx, Results> || ...)
    {
        return std::holds_alternative<Rx>(_result);
    }
};

} // namespace pgb::common