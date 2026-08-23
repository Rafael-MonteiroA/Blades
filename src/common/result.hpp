#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// result.hpp — Result<T, E> type for explicit error handling
//
// Used throughout the Blades compiler instead of exceptions for predictable,
// zero-cost error propagation. Inspired by Rust's Result<T, E>.
//
// Usage:
//   Result<Token, CompileError> lex_next(Source& src);
//
//   auto result = lex_next(src);
//   if (result.is_err()) {
//       report(result.unwrap_err());
//       return;
//   }
//   Token tok = result.unwrap();
// ─────────────────────────────────────────────────────────────────────────────

#include <functional>
#include <stdexcept>
#include <string>
#include <variant>

namespace blades
{

// ─────────────────────────────────────────────────────────────────────────────
// Helper tags for constructing Ok / Err variants unambiguously
// ─────────────────────────────────────────────────────────────────────────────

template<typename T>
struct Ok
{
    T value;

    explicit Ok(T v) : value(std::move(v)) {}
};

template<typename E>
struct Err
{
    E value;

    explicit Err(E v) : value(std::move(v)) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Result<T, E>
// ─────────────────────────────────────────────────────────────────────────────

template<typename T, typename E>
class Result
{
public:
    // ── Construction ─────────────────────────────────────────────────────────

    /// Constructs a successful result.
    /*implicit*/ Result(Ok<T> ok) : m_data(std::move(ok.value)) {}

    /// Constructs an error result.
    /*implicit*/ Result(Err<E> err) : m_data(std::in_place_index<1>, std::move(err.value)) {}

    // ── Queries ──────────────────────────────────────────────────────────────

    [[nodiscard]] bool is_ok() const noexcept
    {
        return m_data.index() == 0;
    }

    [[nodiscard]] bool is_err() const noexcept
    {
        return m_data.index() == 1;
    }

    // ── Value access ─────────────────────────────────────────────────────────

    /// Returns the Ok value. Throws if this is an Err.
    [[nodiscard]] T& unwrap() &
    {
        if (is_err())
            throw std::logic_error("called unwrap() on an Err Result");
        return std::get<0>(m_data);
    }

    [[nodiscard]] const T& unwrap() const&
    {
        if (is_err())
            throw std::logic_error("called unwrap() on an Err Result");
        return std::get<0>(m_data);
    }

    [[nodiscard]] T unwrap() &&
    {
        if (is_err())
            throw std::logic_error("called unwrap() on an Err Result");
        return std::move(std::get<0>(m_data));
    }

    /// Returns the Err value. Throws if this is Ok.
    [[nodiscard]] E& unwrap_err() &
    {
        if (is_ok())
            throw std::logic_error("called unwrap_err() on an Ok Result");
        return std::get<1>(m_data);
    }

    [[nodiscard]] const E& unwrap_err() const&
    {
        if (is_ok())
            throw std::logic_error("called unwrap_err() on an Ok Result");
        return std::get<1>(m_data);
    }

    /// Returns the Ok value or a default if Err.
    [[nodiscard]] T unwrap_or(T default_value) const&
    {
        if (is_ok())
            return std::get<0>(m_data);
        return default_value;
    }

    // ── Transformations ──────────────────────────────────────────────────────

    /// Transforms the Ok value with a function, leaving Err unchanged.
    template<typename F>
    [[nodiscard]] auto map(F&& func) const -> Result<std::invoke_result_t<F, const T&>, E>
    {
        using U = std::invoke_result_t<F, const T&>;
        if (is_ok())
            return Ok<U>(func(std::get<0>(m_data)));
        return Err<E>(std::get<1>(m_data));
    }

    /// Transforms the Err value with a function, leaving Ok unchanged.
    template<typename F>
    [[nodiscard]] auto map_err(F&& func) const -> Result<T, std::invoke_result_t<F, const E&>>
    {
        using F2 = std::invoke_result_t<F, const E&>;
        if (is_err())
            return Err<F2>(func(std::get<1>(m_data)));
        return Ok<T>(std::get<0>(m_data));
    }

private:
    std::variant<T, E> m_data;
};

// ─────────────────────────────────────────────────────────────────────────────
// Convenience factory functions
// ─────────────────────────────────────────────────────────────────────────────

/// Creates an Ok result. Example: return ok(42);
template<typename T>
[[nodiscard]] Ok<T> ok(T value)
{
    return Ok<T>(std::move(value));
}

/// Creates an Err result. Example: return err(CompileError{"undefined variable"});
template<typename E>
[[nodiscard]] Err<E> err(E value)
{
    return Err<E>(std::move(value));
}

} // namespace blades
