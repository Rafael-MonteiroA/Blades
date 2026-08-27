#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// token.hpp — Defines tokens for the Blades lexer
// ─────────────────────────────────────────────────────────────────────────────

#include <string_view>
#include <string>

#include "common/source_location.hpp"

namespace blades
{

// ─────────────────────────────────────────────────────────────────────────────
// TokenType
// ─────────────────────────────────────────────────────────────────────────────

enum class TokenType
{
    // Single-character tokens
    LeftParen, RightParen,
    LeftBrace, RightBrace,
    LeftBracket, RightBracket,
    Comma, Dot, Minus, Plus,
    Semicolon, Slash, Star, Percent,
    Colon, Ampersand, Pipe, Caret, Tilde,

    // One or two character tokens
    Bang, BangEqual,
    Equal, EqualEqual,
    Greater, GreaterEqual, GreaterGreater,
    Less, LessEqual, LessLess,
    Arrow, // ->
    FatArrow, // =>
    AmpAmp, PipePipe, // && ||

    // Compound assignment
    PlusEqual, MinusEqual, StarEqual, SlashEqual, PercentEqual,

    // Literals
    Identifier, String, Integer, Float,

    // Keywords
    Fn, Let, Const, Return, Struct, Class, Match,
    If, Else, True, False, Nil,
    For, While, And, Or, This, Super, Import, Yield,
    Break, Continue, In, Enum,

    // Special
    FString,
    Underscore, // _
    Eof,
    Error
};

// ─────────────────────────────────────────────────────────────────────────────
// Token
// ─────────────────────────────────────────────────────────────────────────────

struct Token
{
    TokenType type;
    std::string_view lexeme;
    SourceSpan span;

    [[nodiscard]] std::string to_string() const;
};

// Returns a string representation of the token type (e.g. "Identifier")
[[nodiscard]] const char* to_string(TokenType type);

} // namespace blades
