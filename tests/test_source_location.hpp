#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// test_source_location.hpp — Tests for SourceLocation and SourceSpan
// ─────────────────────────────────────────────────────────────────────────────

#include "framework/test.hpp"
#include "common/source_location.hpp"

using namespace blades;

TEST_CASE("SourceLocation: defaults to line 1 column 1")
{
    SourceLocation loc;
    ASSERT_EQ(loc.line, 1u);
    ASSERT_EQ(loc.column, 1u);
    ASSERT_EQ(loc.offset, 0u);
}

TEST_CASE("SourceLocation: to_string with filename")
{
    SourceLocation loc;
    loc.line     = 10;
    loc.column   = 5;
    loc.filename = "main.bl";
    ASSERT_EQ(loc.to_string(), std::string("main.bl:10:5"));
}

TEST_CASE("SourceLocation: to_string without filename")
{
    SourceLocation loc;
    loc.line   = 3;
    loc.column = 7;
    ASSERT_EQ(loc.to_string(), std::string("3:7"));
}

TEST_CASE("SourceSpan: length computed correctly")
{
    SourceSpan span;
    span.start.offset = 10;
    span.end.offset   = 20;
    ASSERT_EQ(span.length(), 10u);
}

TEST_CASE("SourceSpan: to_string delegates to start location")
{
    SourceSpan span;
    span.start.line     = 2;
    span.start.column   = 4;
    span.start.filename = "test.bl";
    ASSERT_EQ(span.to_string(), std::string("test.bl:2:4"));
}
