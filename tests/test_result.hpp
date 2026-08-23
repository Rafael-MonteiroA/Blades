#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// test_result.hpp — Tests for Result<T, E>
// ─────────────────────────────────────────────────────────────────────────────

#include "framework/test.hpp"
#include "common/result.hpp"

#include <string>

using namespace blades;

// ── Construction and query ────────────────────────────────────────────────────

TEST_CASE("Result: ok() produces an Ok result")
{
    Result<int, std::string> r = ok(42);
    ASSERT_TRUE(r.is_ok());
    ASSERT_FALSE(r.is_err());
}

TEST_CASE("Result: err() produces an Err result")
{
    Result<int, std::string> r = err(std::string("oops"));
    ASSERT_TRUE(r.is_err());
    ASSERT_FALSE(r.is_ok());
}

// ── Value access ──────────────────────────────────────────────────────────────

TEST_CASE("Result: unwrap() returns the Ok value")
{
    Result<int, std::string> r = ok(99);
    ASSERT_EQ(r.unwrap(), 99);
}

TEST_CASE("Result: unwrap_err() returns the Err value")
{
    Result<int, std::string> r = err(std::string("bad"));
    ASSERT_EQ(r.unwrap_err(), std::string("bad"));
}

TEST_CASE("Result: unwrap() throws on Err")
{
    Result<int, std::string> r = err(std::string("fail"));
    ASSERT_THROWS(r.unwrap());
}

TEST_CASE("Result: unwrap_err() throws on Ok")
{
    Result<int, std::string> r = ok(1);
    ASSERT_THROWS(r.unwrap_err());
}

TEST_CASE("Result: unwrap_or() returns default on Err")
{
    Result<int, std::string> r = err(std::string("nope"));
    ASSERT_EQ(r.unwrap_or(0), 0);
}

TEST_CASE("Result: unwrap_or() returns value on Ok")
{
    Result<int, std::string> r = ok(7);
    ASSERT_EQ(r.unwrap_or(0), 7);
}

// ── Transformations ───────────────────────────────────────────────────────────

TEST_CASE("Result: map() transforms Ok value")
{
    Result<int, std::string> r = ok(3);
    auto doubled = r.map([](const int& v) { return v * 2; });
    ASSERT_TRUE(doubled.is_ok());
    ASSERT_EQ(doubled.unwrap(), 6);
}

TEST_CASE("Result: map() passes Err through unchanged")
{
    Result<int, std::string> r = err(std::string("error"));
    auto mapped = r.map([](const int& v) { return v * 2; });
    ASSERT_TRUE(mapped.is_err());
    ASSERT_EQ(mapped.unwrap_err(), std::string("error"));
}

TEST_CASE("Result: map_err() transforms Err value")
{
    Result<int, std::string> r = err(std::string("oops"));
    auto mapped = r.map_err([](const std::string& e) { return e + "!"; });
    ASSERT_TRUE(mapped.is_err());
    ASSERT_EQ(mapped.unwrap_err(), std::string("oops!"));
}

TEST_CASE("Result: map_err() passes Ok through unchanged")
{
    Result<int, std::string> r = ok(5);
    auto mapped = r.map_err([](const std::string& e) { return e + "!"; });
    ASSERT_TRUE(mapped.is_ok());
    ASSERT_EQ(mapped.unwrap(), 5);
}
