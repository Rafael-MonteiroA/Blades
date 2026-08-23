#pragma once

#include "framework/test.hpp"
#include "compiler/lexer.hpp"
#include <vector>

namespace blades::test
{

inline std::vector<Token> tokenize_all(std::string_view source)
{
    Lexer lexer(source, "test.bl");
    std::vector<Token> tokens;
    while (true)
    {
        Token t = lexer.next_token();
        tokens.push_back(t);
        if (t.type == TokenType::Eof || t.type == TokenType::Error)
        {
            break;
        }
    }
    return tokens;
}

TEST_CASE("Lexer - single character tokens")
{
    auto tokens = tokenize_all("(){}[];,.-+/*:");
    ASSERT_EQ(tokens.size(), 15u);
    ASSERT_EQ(tokens[0].type, TokenType::LeftParen);
    ASSERT_EQ(tokens[1].type, TokenType::RightParen);
    ASSERT_EQ(tokens[2].type, TokenType::LeftBrace);
    ASSERT_EQ(tokens[3].type, TokenType::RightBrace);
    ASSERT_EQ(tokens[4].type, TokenType::LeftBracket);
    ASSERT_EQ(tokens[5].type, TokenType::RightBracket);
    ASSERT_EQ(tokens[6].type, TokenType::Semicolon);
    ASSERT_EQ(tokens[7].type, TokenType::Comma);
    ASSERT_EQ(tokens[8].type, TokenType::Dot);
    ASSERT_EQ(tokens[9].type, TokenType::Minus);
    ASSERT_EQ(tokens[10].type, TokenType::Plus);
    ASSERT_EQ(tokens[11].type, TokenType::Slash);
    ASSERT_EQ(tokens[12].type, TokenType::Star);
    ASSERT_EQ(tokens[13].type, TokenType::Colon);
    ASSERT_EQ(tokens[14].type, TokenType::Eof);
}

TEST_CASE("Lexer - multi character tokens")
{
    auto tokens = tokenize_all("! != = == > >= < <= ->");
    ASSERT_EQ(tokens.size(), 10u);
    ASSERT_EQ(tokens[0].type, TokenType::Bang);
    ASSERT_EQ(tokens[1].type, TokenType::BangEqual);
    ASSERT_EQ(tokens[2].type, TokenType::Equal);
    ASSERT_EQ(tokens[3].type, TokenType::EqualEqual);
    ASSERT_EQ(tokens[4].type, TokenType::Greater);
    ASSERT_EQ(tokens[5].type, TokenType::GreaterEqual);
    ASSERT_EQ(tokens[6].type, TokenType::Less);
    ASSERT_EQ(tokens[7].type, TokenType::LessEqual);
    ASSERT_EQ(tokens[8].type, TokenType::Arrow);
    ASSERT_EQ(tokens[9].type, TokenType::Eof);
}

TEST_CASE("Lexer - literals")
{
    auto tokens = tokenize_all("my_var 123 45.67 \"hello world\"");
    ASSERT_EQ(tokens.size(), 5u);
    
    ASSERT_EQ(tokens[0].type, TokenType::Identifier);
    ASSERT_EQ(tokens[0].lexeme, "my_var");
    
    ASSERT_EQ(tokens[1].type, TokenType::Integer);
    ASSERT_EQ(tokens[1].lexeme, "123");
    
    ASSERT_EQ(tokens[2].type, TokenType::Float);
    ASSERT_EQ(tokens[2].lexeme, "45.67");
    
    ASSERT_EQ(tokens[3].type, TokenType::String);
    ASSERT_EQ(tokens[3].lexeme, "\"hello world\"");
    
    ASSERT_EQ(tokens[4].type, TokenType::Eof);
}

TEST_CASE("Lexer - keywords")
{
    auto tokens = tokenize_all("fn let return struct if else true false for while and or");
    ASSERT_EQ(tokens.size(), 13u);
    
    ASSERT_EQ(tokens[0].type, TokenType::Fn);
    ASSERT_EQ(tokens[1].type, TokenType::Let);
    ASSERT_EQ(tokens[2].type, TokenType::Return);
    ASSERT_EQ(tokens[3].type, TokenType::Struct);
    ASSERT_EQ(tokens[4].type, TokenType::If);
    ASSERT_EQ(tokens[5].type, TokenType::Else);
    ASSERT_EQ(tokens[6].type, TokenType::True);
    ASSERT_EQ(tokens[7].type, TokenType::False);
    ASSERT_EQ(tokens[8].type, TokenType::For);
    ASSERT_EQ(tokens[9].type, TokenType::While);
    ASSERT_EQ(tokens[10].type, TokenType::And);
    ASSERT_EQ(tokens[11].type, TokenType::Or);
    ASSERT_EQ(tokens[12].type, TokenType::Eof);
}

TEST_CASE("Lexer - comments and whitespace")
{
    auto tokens = tokenize_all(" \t\r\n 123 // this is a comment \n 456");
    ASSERT_EQ(tokens.size(), 3u);
    ASSERT_EQ(tokens[0].type, TokenType::Integer);
    ASSERT_EQ(tokens[0].lexeme, "123");
    ASSERT_EQ(tokens[1].type, TokenType::Integer);
    ASSERT_EQ(tokens[1].lexeme, "456");
    ASSERT_EQ(tokens[2].type, TokenType::Eof);
}

TEST_CASE("Lexer - location tracking")
{
    auto tokens = tokenize_all("let x =\n  42;");
    ASSERT_EQ(tokens.size(), 6u);
    
    ASSERT_EQ(tokens[0].lexeme, "let");
    ASSERT_EQ(tokens[0].span.start.line, 1u);
    ASSERT_EQ(tokens[0].span.start.column, 1u);
    ASSERT_EQ(tokens[0].span.end.column, 4u);

    ASSERT_EQ(tokens[1].lexeme, "x");
    ASSERT_EQ(tokens[1].span.start.line, 1u);
    ASSERT_EQ(tokens[1].span.start.column, 5u);

    ASSERT_EQ(tokens[2].lexeme, "=");
    ASSERT_EQ(tokens[2].span.start.line, 1u);
    ASSERT_EQ(tokens[2].span.start.column, 7u);

    ASSERT_EQ(tokens[3].lexeme, "42");
    ASSERT_EQ(tokens[3].span.start.line, 2u);
    ASSERT_EQ(tokens[3].span.start.column, 3u);
    ASSERT_EQ(tokens[3].span.end.column, 5u);

    ASSERT_EQ(tokens[4].type, TokenType::Semicolon);
}

} // namespace blades::test
