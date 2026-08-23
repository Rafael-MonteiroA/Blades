#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// source_location.hpp — Represents a position in Blades source code
//
// Every Token, AST node, and diagnostic carries a SourceLocation so that
// error messages can point precisely to the offending code.
// ─────────────────────────────────────────────────────────────────────────────

#include <string>
#include <string_view>

#include "common/types.hpp"

namespace blades
{

// ─────────────────────────────────────────────────────────────────────────────
// SourceLocation — A single point in a source file (line + column + offset)
// ─────────────────────────────────────────────────────────────────────────────

struct SourceLocation
{
    /// 1-based line number.
    u32 line   = 1;

    /// 1-based column number (byte offset within the line).
    u32 column = 1;

    /// Byte offset from the beginning of the source string.
    u32 offset = 0;

    /// Name of the source file (or "<stdin>", "<repl>", etc.).
    std::string_view filename;

    /// Returns a human-readable representation: "filename:line:column"
    [[nodiscard]] std::string to_string() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// SourceSpan — A range in a source file (start..end, inclusive)
//
// Used by tokens and AST nodes to capture the full extent of a syntactic
// construct, enabling precise underline-style error messages.
// ─────────────────────────────────────────────────────────────────────────────

struct SourceSpan
{
    SourceLocation start;
    SourceLocation end;

    /// Length in bytes of the spanned text.
    [[nodiscard]] u32 length() const noexcept
    {
        return end.offset - start.offset;
    }

    /// Returns "filename:line:col" for the start of the span.
    [[nodiscard]] std::string to_string() const
    {
        return start.to_string();
    }
};

} // namespace blades
