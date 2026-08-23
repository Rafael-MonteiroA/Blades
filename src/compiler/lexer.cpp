#include "compiler/lexer.hpp"

#include <cctype>

namespace blades
{

Lexer::Lexer(std::string_view source, std::string_view filename)
    : m_source(source), m_filename(filename)
{
}

bool Lexer::is_at_end() const
{
    return m_current_offset >= m_source.length();
}

char Lexer::advance()
{
    m_column++;
    return m_source[m_current_offset++];
}

char Lexer::peek() const
{
    if (is_at_end()) return '\0';
    return m_source[m_current_offset];
}

char Lexer::peek_next() const
{
    if (m_current_offset + 1 >= m_source.length()) return '\0';
    return m_source[m_current_offset + 1];
}

bool Lexer::match(char expected)
{
    if (is_at_end()) return false;
    if (m_source[m_current_offset] != expected) return false;
    m_current_offset++;
    m_column++;
    return true;
}

void Lexer::skip_whitespace()
{
    while (true)
    {
        char c = peek();
        switch (c)
        {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                m_line++;
                m_column = 0; // next advance will make it 1
                advance();
                break;
            case '/':
                if (peek_next() == '/')
                {
                    // A comment goes until the end of the line.
                    while (peek() != '\n' && !is_at_end()) advance();
                }
                else
                {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

Token Lexer::make_token(TokenType type)
{
    SourceSpan span;
    span.start = SourceLocation{ m_line, m_start_column, m_start_offset, m_filename };
    span.end = SourceLocation{ m_line, m_column, m_current_offset, m_filename };

    std::string_view lexeme = m_source.substr(m_start_offset, m_current_offset - m_start_offset);

    return Token{ type, lexeme, span };
}

Token Lexer::error_token(std::string_view message)
{
    SourceSpan span;
    span.start = SourceLocation{ m_line, m_start_column, m_start_offset, m_filename };
    span.end = SourceLocation{ m_line, m_column, m_current_offset, m_filename };

    return Token{ TokenType::Error, message, span };
}

Token Lexer::next_token()
{
    skip_whitespace();
    
    m_start_offset = m_current_offset;
    m_start_column = m_column;

    if (is_at_end()) return make_token(TokenType::Eof);

    char c = advance();

    if (std::isalpha(c) || c == '_') return identifier();
    if (std::isdigit(c)) return number();

    switch (c)
    {
        case '(': return make_token(TokenType::LeftParen);
        case ')': return make_token(TokenType::RightParen);
        case '{': return make_token(TokenType::LeftBrace);
        case '}': return make_token(TokenType::RightBrace);
        case '[': return make_token(TokenType::LeftBracket);
        case ']': return make_token(TokenType::RightBracket);
        case ';': return make_token(TokenType::Semicolon);
        case ',': return make_token(TokenType::Comma);
        case '.': return make_token(TokenType::Dot);
        case '-': 
            if (match('>')) return make_token(TokenType::Arrow);
            return make_token(TokenType::Minus);
        case '+': return make_token(TokenType::Plus);
        case '/': return make_token(TokenType::Slash);
        case '*': return make_token(TokenType::Star);
        case ':': return make_token(TokenType::Colon);
        case '!':
            return make_token(match('=') ? TokenType::BangEqual : TokenType::Bang);
        case '=':
            return make_token(match('=') ? TokenType::EqualEqual : TokenType::Equal);
        case '<':
            return make_token(match('=') ? TokenType::LessEqual : TokenType::Less);
        case '>':
            return make_token(match('=') ? TokenType::GreaterEqual : TokenType::Greater);
        case '"': return string();
    }

    return error_token("Unexpected character.");
}

Token Lexer::string()
{
    while (peek() != '"' && !is_at_end())
    {
        if (peek() == '\n') 
        {
            m_line++;
            m_column = 0;
        }
        advance();
    }

    if (is_at_end()) return error_token("Unterminated string.");

    // The closing quote.
    advance();
    return make_token(TokenType::String);
}

Token Lexer::number()
{
    while (std::isdigit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && std::isdigit(peek_next()))
    {
        // Consume the "."
        advance();

        while (std::isdigit(peek())) advance();
        return make_token(TokenType::Float);
    }

    return make_token(TokenType::Integer);
}

Token Lexer::identifier()
{
    while (std::isalnum(peek()) || peek() == '_') advance();
    return make_token(identifier_type());
}

TokenType Lexer::check_keyword(u32 start, u32 length, std::string_view rest, TokenType type) const
{
    u32 current_len = m_current_offset - m_start_offset;
    if (current_len == start + length && 
        m_source.substr(m_start_offset + start, length) == rest)
    {
        return type;
    }
    return TokenType::Identifier;
}

TokenType Lexer::identifier_type() const
{
    // A simple trie or switch for keywords.
    char c = m_source[m_start_offset];
    switch (c)
    {
        case 'a': return check_keyword(1, 2, "nd", TokenType::And);
        case 'e': return check_keyword(1, 3, "lse", TokenType::Else);
        case 'f':
            if (m_current_offset - m_start_offset > 1) {
                switch (m_source[m_start_offset + 1]) {
                    case 'a': return check_keyword(2, 3, "lse", TokenType::False);
                    case 'n': return check_keyword(2, 0, "", TokenType::Fn);
                    case 'o': return check_keyword(2, 1, "r", TokenType::For);
                }
            }
            break;
        case 'i': return check_keyword(1, 1, "f", TokenType::If);
        case 'l': return check_keyword(1, 2, "et", TokenType::Let);
        case 'o': return check_keyword(1, 1, "r", TokenType::Or);
        case 'r': return check_keyword(1, 5, "eturn", TokenType::Return);
        case 's': return check_keyword(1, 5, "truct", TokenType::Struct);
        case 't': return check_keyword(1, 3, "rue", TokenType::True);
        case 'w': return check_keyword(1, 4, "hile", TokenType::While);
    }
    return TokenType::Identifier;
}

} // namespace blades
