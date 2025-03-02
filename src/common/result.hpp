#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>
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
    constexpr Result(bool isSuccess) : _isSuccess(isSuccess) {}
    static constexpr const char* GetDescription() { return Description.value; }

    constexpr bool IsSuccess() const { return _isSuccess; }
    constexpr bool IsFailure() const { return !_isSuccess; }
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
    constexpr ResultSet(const ResultOptions& result, const Type& value) : _result(result), _value(value) {}

    constexpr bool IsSuccess() const
    {
        static_assert(std::size(_isSuccessVisitor) == sizeof...(Results));
        return _isSuccessVisitor[_result.index()](_result);
    }

    constexpr bool IsFailure() const
    {
        static_assert(std::size(_isFailureVisitor) == sizeof...(Results));
        return _isFailureVisitor[_result.index()](_result);
    }

    constexpr const char* GetStatusDescription() const
    {
        static_assert(std::size(_descriptions) == sizeof...(Results));
        return _descriptions[_result.index()];
    }

    template <typename Rx>
    constexpr bool IsResult() const
        requires(std::is_same_v<Rx, Results> || ...)
    {
        return std::holds_alternative<Rx>(_result);
    }
};

} // namespace pgb::common