#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// parser.hpp — Recursive Descent Parser for Blades
// ─────────────────────────────────────────────────────────────────────────────

#include <vector>
#include <memory>
#include <stdexcept>

#include "compiler/lexer.hpp"
#include "compiler/ast.hpp"

namespace blades
{

class ParseError : public std::runtime_error
{
public:
    explicit ParseError(const std::string& message) : std::runtime_error(message) {}
};

class Parser
{
public:
    explicit Parser(Lexer& lexer);

    // Parses a sequence of statements until EOF.
    std::vector<std::unique_ptr<Stmt>> parse();

    [[nodiscard]] bool had_error() const noexcept { return m_had_error; }

private:
    Lexer& m_lexer;
    Token m_current;
    Token m_previous;
    bool m_had_error = false;
    bool m_panic_mode = false;

    // Helpers
    void advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    void consume(TokenType type, const char* message);
    
    void error_at_current(const char* message);
    void error(const char* message);
    void synchronize();

    // Grammar - Statements
    std::unique_ptr<Stmt> declaration();
    std::unique_ptr<Stmt> class_declaration();
    std::unique_ptr<Stmt> fn_declaration(std::string kind);
    std::unique_ptr<Stmt> let_declaration(bool is_const = false);
    std::unique_ptr<Stmt> import_statement();
    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Stmt> if_statement();
    std::unique_ptr<Stmt> return_statement();
    std::unique_ptr<Stmt> break_statement();
    std::unique_ptr<Stmt> continue_statement();
    std::unique_ptr<Stmt> while_statement();
    std::unique_ptr<Stmt> for_statement();
    std::unique_ptr<Stmt> block_statement();
    std::unique_ptr<Stmt> expression_statement();

    // Grammar - Expressions
    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> match_expression();
    std::unique_ptr<Expr> assignment();
    std::unique_ptr<Expr> logical_or();
    std::unique_ptr<Expr> logical_and();
    std::unique_ptr<Expr> bitwise_or();
    std::unique_ptr<Expr> bitwise_xor();
    std::unique_ptr<Expr> bitwise_and();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> shift();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> call();
    std::unique_ptr<Expr> primary();
    std::unique_ptr<Expr> parse_fstring(Token token);

    // Loop tracking for break/continue
    int m_loop_depth = 0;
};

} // namespace blades
