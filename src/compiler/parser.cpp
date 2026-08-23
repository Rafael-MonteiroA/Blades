#include "compiler/parser.hpp"

#include <iostream>

namespace blades
{

Parser::Parser(Lexer& lexer) : m_lexer(lexer)
{
    // Initialize tokens
    m_current = m_lexer.next_token();
    m_previous = m_current; // Just to have it initialized
}

void Parser::advance()
{
    m_previous = m_current;
    
    while (true)
    {
        m_current = m_lexer.next_token();
        if (m_current.type != TokenType::Error)
            break;
            
        error_at_current(std::string(m_current.lexeme).c_str());
    }
}

bool Parser::check(TokenType type) const
{
    return m_current.type == type;
}

bool Parser::match(TokenType type)
{
    if (!check(type)) return false;
    advance();
    return true;
}

void Parser::consume(TokenType type, const char* message)
{
    if (m_current.type == type)
    {
        advance();
        return;
    }
    error_at_current(message);
}

void Parser::error_at_current(const char* message)
{
    if (m_panic_mode) return;
    m_panic_mode = true;
    m_had_error = true;
    
    std::cerr << "[line " << m_current.span.start.line << "] Error";
    if (m_current.type == TokenType::Eof) {
        std::cerr << " at end";
    } else if (m_current.type == TokenType::Error) {
        // Nothing
    } else {
        std::cerr << " at '" << m_current.lexeme << "'";
    }
    std::cerr << ": " << message << "\n";
    
    throw ParseError(message);
}

void Parser::error(const char* message)
{
    // For now we just use error_at_current which uses m_current.
    // In a real compiler we'd report at m_previous for things just consumed.
    if (m_panic_mode) return;
    m_panic_mode = true;
    m_had_error = true;
    std::cerr << "[line " << m_previous.span.start.line << "] Error at '" << m_previous.lexeme << "': " << message << "\n";
    throw ParseError(message);
}

void Parser::synchronize()
{
    m_panic_mode = false;
    
    while (m_current.type != TokenType::Eof)
    {
        if (m_previous.type == TokenType::Semicolon) return;
        
        switch (m_current.type)
        {
            case TokenType::Struct:
            case TokenType::Fn:
            case TokenType::Let:
            case TokenType::For:
            case TokenType::If:
            case TokenType::While:
            case TokenType::Return:
                return;
            default:
                break;
        }
        advance();
    }
}

std::vector<std::unique_ptr<Stmt>> Parser::parse()
{
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!check(TokenType::Eof))
    {
        try
        {
            statements.push_back(declaration());
        }
        catch (const ParseError&)
        {
            synchronize();
        }
    }
    return statements;
}

// ─────────────────────────────────────────────────────────────────────────────
// Statements
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<Stmt> Parser::declaration()
{
    if (match(TokenType::Fn)) return function_declaration();
    if (match(TokenType::Let)) return let_declaration();
    return statement();
}

std::unique_ptr<Stmt> Parser::function_declaration()
{
    consume(TokenType::Identifier, "Expect function name.");
    Token name = m_previous;
    
    consume(TokenType::LeftParen, "Expect '(' after function name.");
    std::vector<Token> parameters;
    if (!check(TokenType::RightParen))
    {
        do
        {
            consume(TokenType::Identifier, "Expect parameter name.");
            parameters.push_back(m_previous);
            
            // Temporary syntax for types: name: type
            if (match(TokenType::Colon))
            {
                consume(TokenType::Identifier, "Expect parameter type.");
                // We ignore the type in AST for now, or we'd store it in a TypedToken
            }
        } while (match(TokenType::Comma));
    }
    consume(TokenType::RightParen, "Expect ')' after parameters.");
    
    if (match(TokenType::Arrow))
    {
        consume(TokenType::Identifier, "Expect return type.");
    }
    
    consume(TokenType::LeftBrace, "Expect '{' before function body.");
    auto body = std::unique_ptr<BlockStmt>(static_cast<BlockStmt*>(block_statement().release()));
    
    return std::make_unique<FunctionDecl>(std::move(name), std::move(parameters), std::move(body));
}

std::unique_ptr<Stmt> Parser::let_declaration()
{
    consume(TokenType::Identifier, "Expect variable name.");
    Token name = m_previous;
    
    std::unique_ptr<Expr> initializer = nullptr;
    if (match(TokenType::Equal))
    {
        initializer = expression();
    }
    
    consume(TokenType::Semicolon, "Expect ';' after variable declaration.");
    return std::make_unique<LetStmt>(std::move(name), std::move(initializer));
}

std::unique_ptr<Stmt> Parser::statement()
{
    if (match(TokenType::If)) return if_statement();
    if (match(TokenType::Return)) return return_statement();
    if (match(TokenType::While)) return while_statement();
    if (match(TokenType::LeftBrace)) return block_statement();
    
    return expression_statement();
}

std::unique_ptr<Stmt> Parser::if_statement()
{
    consume(TokenType::LeftParen, "Expect '(' after 'if'.");
    auto condition = expression();
    consume(TokenType::RightParen, "Expect ')' after if condition.");
    
    auto then_branch = statement();
    std::unique_ptr<Stmt> else_branch = nullptr;
    if (match(TokenType::Else))
    {
        else_branch = statement();
    }
    
    return std::make_unique<IfStmt>(std::move(condition), std::move(then_branch), std::move(else_branch));
}

std::unique_ptr<Stmt> Parser::return_statement()
{
    Token keyword = m_previous;
    std::unique_ptr<Expr> value = nullptr;
    
    if (!check(TokenType::Semicolon))
    {
        value = expression();
    }
    
    consume(TokenType::Semicolon, "Expect ';' after return value.");
    return std::make_unique<ReturnStmt>(std::move(keyword), std::move(value));
}

std::unique_ptr<Stmt> Parser::while_statement()
{
    consume(TokenType::LeftParen, "Expect '(' after 'while'.");
    auto condition = expression();
    consume(TokenType::RightParen, "Expect ')' after condition.");
    auto body = statement();
    
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::block_statement()
{
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!check(TokenType::RightBrace) && !check(TokenType::Eof))
    {
        statements.push_back(declaration());
    }
    consume(TokenType::RightBrace, "Expect '}' after block.");
    return std::make_unique<BlockStmt>(std::move(statements));
}

std::unique_ptr<Stmt> Parser::expression_statement()
{
    auto expr = expression();
    consume(TokenType::Semicolon, "Expect ';' after expression.");
    return std::make_unique<ExprStmt>(std::move(expr));
}

// ─────────────────────────────────────────────────────────────────────────────
// Expressions
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<Expr> Parser::expression()
{
    return assignment();
}

std::unique_ptr<Expr> Parser::assignment()
{
    auto expr = logical_or();
    
    if (match(TokenType::Equal))
    {
        Token equals = m_previous;
        auto value = assignment();
        
        if (auto* var_expr = dynamic_cast<VariableExpr*>(expr.get()))
        {
            Token name = var_expr->name;
            return std::make_unique<AssignExpr>(std::move(name), std::move(value));
        }
        
        error("Invalid assignment target.");
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::logical_or()
{
    auto expr = logical_and();
    while (match(TokenType::Or))
    {
        Token op = m_previous;
        auto right = logical_and();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::logical_and()
{
    auto expr = equality();
    while (match(TokenType::And))
    {
        Token op = m_previous;
        auto right = equality();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::equality()
{
    auto expr = comparison();
    
    while (match(TokenType::BangEqual) || match(TokenType::EqualEqual))
    {
        Token op = m_previous;
        auto right = comparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::comparison()
{
    auto expr = term();
    
    while (match(TokenType::Greater) || match(TokenType::GreaterEqual) ||
           match(TokenType::Less) || match(TokenType::LessEqual))
    {
        Token op = m_previous;
        auto right = term();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::term()
{
    auto expr = factor();
    
    while (match(TokenType::Minus) || match(TokenType::Plus))
    {
        Token op = m_previous;
        auto right = factor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::factor()
{
    auto expr = unary();
    
    while (match(TokenType::Slash) || match(TokenType::Star))
    {
        Token op = m_previous;
        auto right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::unary()
{
    if (match(TokenType::Bang) || match(TokenType::Minus))
    {
        Token op = m_previous;
        auto right = unary();
        return std::make_unique<UnaryExpr>(std::move(op), std::move(right));
    }
    
    return call();
}

std::unique_ptr<Expr> Parser::call()
{
    auto expr = primary();
    
    while (true)
    {
        if (match(TokenType::LeftParen))
        {
            std::vector<std::unique_ptr<Expr>> arguments;
            if (!check(TokenType::RightParen))
            {
                do
                {
                    arguments.push_back(expression());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RightParen, "Expect ')' after arguments.");
            Token paren = m_previous;
            expr = std::make_unique<CallExpr>(std::move(expr), std::move(paren), std::move(arguments));
        }
        else
        {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::primary()
{
    if (match(TokenType::False)) return std::make_unique<LiteralExpr>(m_previous);
    if (match(TokenType::True)) return std::make_unique<LiteralExpr>(m_previous);
    if (match(TokenType::Integer) || match(TokenType::Float) || match(TokenType::String))
    {
        return std::make_unique<LiteralExpr>(m_previous);
    }
    
    if (match(TokenType::Identifier))
    {
        return std::make_unique<VariableExpr>(m_previous);
    }
    
    if (match(TokenType::LeftParen))
    {
        auto expr = expression();
        consume(TokenType::RightParen, "Expect ')' after expression.");
        return std::make_unique<GroupingExpr>(std::move(expr));
    }
    
    error_at_current("Expect expression.");
    return nullptr; // Unreachable
}

} // namespace blades
