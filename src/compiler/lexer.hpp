#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// lexer.hpp — Blades lexical analyzer
// ─────────────────────────────────────────────────────────────────────────────

#include <string_view>

#include "compiler/token.hpp"

namespace blades
{

class Lexer
{
public:
    Lexer(std::string_view source, std::string_view filename);

    /// Fetches the next token from the source stream.
    /// Returns a token with type TokenType::Eof when the end is reached.
    Token next_token();

private:
    std::string_view m_source;
    std::string_view m_filename;

    u32 m_start_offset = 0;
    u32 m_current_offset = 0;
    
    u32 m_line = 1;
    u32 m_column = 1;
    u32 m_start_line = 1;
    u32 m_start_column = 1;

    // Helpers
    bool is_at_end() const;
    char advance();
    char peek() const;
    char peek_next() const;
    bool match(char expected);
    
    void skip_whitespace();
    
    Token make_token(TokenType type);
    Token error_token(std::string_view message);

    // Parsers
    Token string();
    Token fstring();
    Token number();
    Token identifier();
    
    TokenType check_keyword(u32 start, u32 length, std::string_view rest, TokenType type) const;
    TokenType identifier_type() const;
};

} // namespace blades
