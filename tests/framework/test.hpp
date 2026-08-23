#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// test.hpp — Blades micro test framework
//
// A minimal, self-contained test framework with zero external dependencies.
// Tests are registered at static-initialization time and run by calling
// blades::test::run_all().
//
// Usage:
//   #include "framework/test.hpp"
//
//   TEST_CASE("my feature works")
//   {
//       int x = 2 + 2;
//       ASSERT_EQ(x, 4);
//       ASSERT_TRUE(x > 0);
//   }
// ─────────────────────────────────────────────────────────────────────────────

#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace blades::test
{

// ─────────────────────────────────────────────────────────────────────────────
// AssertionFailure — thrown internally when an ASSERT_* macro fails
// ─────────────────────────────────────────────────────────────────────────────

struct AssertionFailure
{
    std::string message;
    const char* file;
    int         line;
};

// ─────────────────────────────────────────────────────────────────────────────
// TestCase — metadata + body of a single test
// ─────────────────────────────────────────────────────────────────────────────

struct TestCase
{
    std::string          name;
    std::function<void()> body;
};

// ─────────────────────────────────────────────────────────────────────────────
// Registry — global list of all registered tests
// ─────────────────────────────────────────────────────────────────────────────

inline std::vector<TestCase>& registry()
{
    static std::vector<TestCase> s_tests;
    return s_tests;
}

// ─────────────────────────────────────────────────────────────────────────────
// Registrar — helper used by TEST_CASE macro
// ─────────────────────────────────────────────────────────────────────────────

struct Registrar
{
    Registrar(std::string name, std::function<void()> body)
    {
        registry().push_back({ std::move(name), std::move(body) });
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// run_all — executes every registered test and prints a summary
//
// Returns EXIT_SUCCESS (0) if all tests pass, EXIT_FAILURE (1) otherwise.
// ─────────────────────────────────────────────────────────────────────────────

inline int run_all()
{
    auto& tests = registry();

    int passed = 0;
    int failed = 0;

    std::cout << "\n── Blades Test Suite ────────────────────────────────────\n";
    std::cout << "  Running " << tests.size() << " test(s)...\n\n";

    for (auto& tc : tests)
    {
        try
        {
            tc.body();
            std::cout << "  \033[32m✓\033[0m " << tc.name << '\n';
            ++passed;
        }
        catch (const AssertionFailure& af)
        {
            std::cout << "  \033[31m✗\033[0m " << tc.name << '\n';
            std::cout << "      " << af.file << ':' << af.line << ": " << af.message << '\n';
            ++failed;
        }
        catch (const std::exception& ex)
        {
            std::cout << "  \033[31m✗\033[0m " << tc.name << '\n';
            std::cout << "      Unexpected exception: " << ex.what() << '\n';
            ++failed;
        }
        catch (...)
        {
            std::cout << "  \033[31m✗\033[0m " << tc.name << '\n';
            std::cout << "      Unknown exception thrown\n";
            ++failed;
        }
    }

    std::cout << "\n────────────────────────────────────────────────────────\n";
    std::cout << "  \033[32m" << passed << " passed\033[0m";
    if (failed > 0)
        std::cout << "  \033[31m" << failed << " failed\033[0m";
    std::cout << "  (total: " << (passed + failed) << ")\n\n";

    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal assertion helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace detail
{

inline void assert_true(bool condition, const char* expr, const char* file, int line)
{
    if (!condition)
    {
        std::ostringstream oss;
        oss << "ASSERT_TRUE(" << expr << ") failed";
        throw AssertionFailure{ oss.str(), file, line };
    }
}

inline void assert_false(bool condition, const char* expr, const char* file, int line)
{
    if (condition)
    {
        std::ostringstream oss;
        oss << "ASSERT_FALSE(" << expr << ") failed";
        throw AssertionFailure{ oss.str(), file, line };
    }
}

template<typename A, typename B>
void assert_eq(const A& a, const B& b, const char* a_expr, const char* b_expr,
               const char* file, int line)
{
    if (!(a == b))
    {
        std::ostringstream oss;
        oss << "ASSERT_EQ(" << a_expr << ", " << b_expr << ") failed";
        throw AssertionFailure{ oss.str(), file, line };
    }
}

template<typename A, typename B>
void assert_ne(const A& a, const B& b, const char* a_expr, const char* b_expr,
               const char* file, int line)
{
    if (a == b)
    {
        std::ostringstream oss;
        oss << "ASSERT_NE(" << a_expr << ", " << b_expr << ") failed: both are equal";
        throw AssertionFailure{ oss.str(), file, line };
    }
}

} // namespace detail
} // namespace blades::test

// ─────────────────────────────────────────────────────────────────────────────
// Public macros
// ─────────────────────────────────────────────────────────────────────────────

#define BLADES_CONCAT_IMPL(a, b) a##b
#define BLADES_CONCAT(a, b) BLADES_CONCAT_IMPL(a, b)

#define BLADES_TEST_CASE_IMPL(name, id)                                              \
    static void BLADES_CONCAT(BLADES_TEST_BODY_, id)();                              \
    static ::blades::test::Registrar BLADES_CONCAT(BLADES_TEST_REG_, id)(            \
        name, BLADES_CONCAT(BLADES_TEST_BODY_, id));                                 \
    static void BLADES_CONCAT(BLADES_TEST_BODY_, id)()

/// Registers a test case. Body follows in braces.
#define TEST_CASE(name) BLADES_TEST_CASE_IMPL(name, __COUNTER__)

/// Asserts that expr evaluates to true.
#define ASSERT_TRUE(expr) \
    ::blades::test::detail::assert_true(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

/// Asserts that expr evaluates to false.
#define ASSERT_FALSE(expr) \
    ::blades::test::detail::assert_false(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

/// Asserts that a == b.
#define ASSERT_EQ(a, b) \
    ::blades::test::detail::assert_eq((a), (b), #a, #b, __FILE__, __LINE__)

/// Asserts that a != b.
#define ASSERT_NE(a, b) \
    ::blades::test::detail::assert_ne((a), (b), #a, #b, __FILE__, __LINE__)

/// Asserts that expr throws any exception.
#define ASSERT_THROWS(expr)                                                      \
    do {                                                                         \
        bool _threw = false;                                                     \
        try { static_cast<void>(expr); } catch (...) { _threw = true; }                          \
        if (!_threw)                                                             \
            throw ::blades::test::AssertionFailure{                              \
                "ASSERT_THROWS(" #expr ") — no exception was thrown",            \
                __FILE__, __LINE__};                                             \
    } while (false)
