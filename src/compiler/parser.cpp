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
            case TokenType::Class:
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

std::unique_ptr<Expr> Parser::parse_fstring(Token token)
{
    std::string_view lexeme = token.lexeme;
    // skip f" and "
    std::string_view content = lexeme.substr(2, lexeme.size() - 3);
    
    std::unique_ptr<Expr> root = nullptr;
    
    size_t i = 0;
    while (i < content.size())
    {
        size_t start = i;
        while (i < content.size() && content[i] != '{') i++;
        
        std::string_view str_part = content.substr(start, i - start);
        
        if (!str_part.empty() || root == nullptr) {
            Token str_tok{TokenType::String, str_part, token.span};
            auto str_expr = std::make_unique<LiteralExpr>(str_tok);
            
            if (!root) {
                root = std::move(str_expr);
            } else {
                Token plus{TokenType::Plus, "+", token.span};
                root = std::make_unique<BinaryExpr>(std::move(root), plus, std::move(str_expr));
            }
        }
        
        if (i < content.size() && content[i] == '{')
        {
            i++; // skip {
            size_t expr_start = i;
            int depth = 1;
            while (i < content.size() && depth > 0)
            {
                if (content[i] == '{') depth++;
                else if (content[i] == '}') depth--;
                i++;
            }
            if (depth > 0) {
                error_at_current("Unterminated '{' in f-string.");
                break;
            }
            
            std::string_view expr_str = content.substr(expr_start, i - expr_start - 1);
            Lexer sub_lexer(expr_str, "fstring");
            Parser sub_parser(sub_lexer);
            auto inner_expr = sub_parser.expression();
            
            if (inner_expr) {
                if (!root) {
                    root = std::move(inner_expr);
                } else {
                    Token plus{TokenType::Plus, "+", token.span};
                    root = std::make_unique<BinaryExpr>(std::move(root), plus, std::move(inner_expr));
                }
            }
        }
    }
    
    if (!root) {
        Token empty_str{TokenType::String, lexeme.substr(2,0), token.span};
        return std::make_unique<LiteralExpr>(empty_str);
    }
    
    return root;
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
    try
    {
        if (match(TokenType::Import)) return import_statement();
        if (match(TokenType::Class)) return class_declaration();
        if (match(TokenType::Fn)) return fn_declaration("function");
        if (match(TokenType::Let)) return let_declaration();
        return statement();
    }
    catch (const ParseError&)
    {
        synchronize();
        return nullptr;
    }
}

std::unique_ptr<Stmt> Parser::fn_declaration(std::string kind)
{
    consume(TokenType::Identifier, ("Expect " + kind + " name.").c_str());
    Token name = m_previous;
    
    consume(TokenType::LeftParen, ("Expect '(' after " + kind + " name.").c_str());
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
    
    consume(TokenType::LeftBrace, ("Expect '{' before " + kind + " body.").c_str());
    auto body = std::unique_ptr<BlockStmt>(static_cast<BlockStmt*>(block_statement().release()));
    
    return std::make_unique<FunctionDecl>(std::move(name), std::move(parameters), std::move(body));
}

std::unique_ptr<Stmt> Parser::class_declaration()
{
    consume(TokenType::Identifier, "Expect class name.");
    Token name = m_previous;
    
    std::unique_ptr<VariableExpr> superclass = nullptr;
    if (match(TokenType::Less))
    {
        consume(TokenType::Identifier, "Expect superclass name.");
        superclass = std::make_unique<VariableExpr>(m_previous);
    }
    
    consume(TokenType::LeftBrace, "Expect '{' before class body.");
    
    std::vector<std::unique_ptr<FunctionDecl>> methods;
    while (!check(TokenType::RightBrace) && !check(TokenType::Eof))
    {
        if (match(TokenType::Fn)) {
            auto method = fn_declaration("method");
            methods.push_back(std::unique_ptr<FunctionDecl>(static_cast<FunctionDecl*>(method.release())));
        } else {
            error_at_current("Expect 'fn' for method declaration.");
        }
    }
    
    consume(TokenType::RightBrace, "Expect '}' after class body.");
    return std::make_unique<ClassDecl>(std::move(name), std::move(superclass), std::move(methods));
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

std::unique_ptr<Stmt> Parser::import_statement()
{
    Token keyword = m_previous;
    consume(TokenType::String, "Expect module path string after 'import'.");
    Token path = m_previous;
    consume(TokenType::Semicolon, "Expect ';' after import path.");
    return std::make_unique<ImportStmt>(std::move(keyword), std::move(path));
}

std::unique_ptr<Stmt> Parser::statement()
{
    if (match(TokenType::If)) return if_statement();
    if (match(TokenType::Return)) return return_statement();
    if (match(TokenType::While)) return while_statement();
    if (match(TokenType::For)) return for_statement();
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

std::unique_ptr<Stmt> Parser::for_statement()
{
    consume(TokenType::LeftParen, "Expect '(' after 'for'.");
    
    std::unique_ptr<Stmt> initializer = nullptr;
    if (match(TokenType::Semicolon))
    {
        initializer = nullptr;
    }
    else if (match(TokenType::Let))
    {
        initializer = let_declaration();
    }
    else
    {
        initializer = expression_statement();
    }
    
    std::unique_ptr<Expr> condition = nullptr;
    if (!check(TokenType::Semicolon))
    {
        condition = expression();
    }
    consume(TokenType::Semicolon, "Expect ';' after loop condition.");
    
    std::unique_ptr<Expr> increment = nullptr;
    if (!check(TokenType::RightParen))
    {
        increment = expression();
    }
    consume(TokenType::RightParen, "Expect ')' after for clauses.");
    
    auto body = statement();
    
    return std::make_unique<ForStmt>(std::move(initializer), std::move(condition), std::move(increment), std::move(body));
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
    if (match(TokenType::Yield))
    {
        Token keyword = m_previous;
        std::unique_ptr<Expr> value = nullptr;
        // Se não for o fim da expressão, tentamos parsear o valor
        if (!check(TokenType::Semicolon) && !check(TokenType::RightParen) && !check(TokenType::RightBrace) && !check(TokenType::Comma))
        {
            value = assignment();
        }
        return std::make_unique<YieldExpr>(std::move(keyword), std::move(value));
    }
    if (match(TokenType::Match))
    {
        return match_expression();
    }
    return assignment();
}

std::unique_ptr<Expr> Parser::match_expression()
{
    Token keyword = m_previous;
    consume(TokenType::LeftParen, "Expect '(' after 'match'.");
    auto value = expression();
    consume(TokenType::RightParen, "Expect ')' after match value.");
    consume(TokenType::LeftBrace, "Expect '{' before match arms.");
    
    std::vector<MatchArm> arms;
    while (!check(TokenType::RightBrace) && !check(TokenType::Eof))
    {
        std::unique_ptr<Expr> pattern = nullptr;
        if (match(TokenType::Underscore))
        {
            // default fallback, pattern remains nullptr
        }
        else
        {
            pattern = expression();
        }
        
        consume(TokenType::FatArrow, "Expect '=>' after match pattern.");
        
        auto body = expression();
        arms.emplace_back(std::move(pattern), std::move(body));
        
        if (!match(TokenType::Comma))
        {
            break;
        }
    }
    
    consume(TokenType::RightBrace, "Expect '}' after match arms.");
    return std::make_unique<MatchExpr>(std::move(keyword), std::move(value), std::move(arms));
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
            return std::make_unique<AssignExpr>(var_expr->name, std::move(value));
        }
        else if (auto* sub_expr = dynamic_cast<SubscriptExpr*>(expr.get()))
        {
            return std::make_unique<SubscriptAssignExpr>(
                std::move(sub_expr->object), 
                std::move(sub_expr->index), 
                std::move(value),
                Token{} // fake bracket token
            );
        }
        else if (auto* prop_expr = dynamic_cast<PropertyExpr*>(expr.get()))
        {
            return std::make_unique<PropertyAssignExpr>(
                std::move(prop_expr->object), 
                prop_expr->name, 
                std::move(value)
            );
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
    auto expr = bitwise_or();
    while (match(TokenType::And))
    {
        Token op = m_previous;
        auto right = bitwise_or();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::bitwise_or()
{
    auto expr = bitwise_xor();
    while (match(TokenType::Pipe))
    {
        Token op = m_previous;
        auto right = bitwise_xor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::bitwise_xor()
{
    auto expr = bitwise_and();
    while (match(TokenType::Caret))
    {
        Token op = m_previous;
        auto right = bitwise_and();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::bitwise_and()
{
    auto expr = equality();
    while (match(TokenType::Ampersand))
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
    auto expr = shift();
    
    while (match(TokenType::Greater) || match(TokenType::GreaterEqual) ||
           match(TokenType::Less) || match(TokenType::LessEqual))
    {
        Token op = m_previous;
        auto right = shift();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::shift()
{
    auto expr = term();
    
    while (match(TokenType::LessLess) || match(TokenType::GreaterGreater))
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
    if (match(TokenType::Bang) || match(TokenType::Minus) || match(TokenType::Tilde))
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
        else if (match(TokenType::LeftBracket))
        {
            auto index = expression();
            consume(TokenType::RightBracket, "Expect ']' after index.");
            expr = std::make_unique<SubscriptExpr>(std::move(expr), std::move(index));
        }
        else if (match(TokenType::Dot))
        {
            consume(TokenType::Identifier, "Expect property name after '.'.");
            Token name = m_previous;
            expr = std::make_unique<PropertyExpr>(std::move(expr), std::move(name));
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
    
    if (match(TokenType::This))
    {
        return std::make_unique<ThisExpr>(m_previous);
    }
    
    if (match(TokenType::Fn))
    {
        consume(TokenType::LeftParen, "Expect '(' after 'fn'.");
        std::vector<Token> parameters;
        if (!check(TokenType::RightParen))
        {
            do
            {
                consume(TokenType::Identifier, "Expect parameter name.");
                parameters.push_back(m_previous);
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RightParen, "Expect ')' after parameters.");
        
        consume(TokenType::LeftBrace, "Expect '{' before function body.");
        auto body = std::unique_ptr<BlockStmt>(static_cast<BlockStmt*>(block_statement().release()));
        
        return std::make_unique<FnExpr>(std::move(parameters), std::move(body));
    }
    
    if (match(TokenType::Super))
    {
        Token keyword = m_previous;
        consume(TokenType::Dot, "Expect '.' after 'super'.");
        consume(TokenType::Identifier, "Expect superclass method name.");
        Token method = m_previous;
        return std::make_unique<SuperExpr>(std::move(keyword), std::move(method));
    }

    if (match(TokenType::FString))
    {
        return parse_fstring(m_previous);
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
    
    if (match(TokenType::LeftBracket))
    {
        std::vector<std::unique_ptr<Expr>> elements;
        if (!check(TokenType::RightBracket))
        {
            do
            {
                elements.push_back(expression());
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RightBracket, "Expect ']' after array elements.");
        return std::make_unique<ArrayExpr>(std::move(elements));
    }
    
    if (match(TokenType::LeftBrace))
    {
        std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> elements;
        if (!check(TokenType::RightBrace))
        {
            do
            {
                std::unique_ptr<Expr> key;
                if (match(TokenType::Identifier) || match(TokenType::String))
                {
                    Token key_token = m_previous;
                    if (key_token.type == TokenType::Identifier) {
                        key_token.type = TokenType::String; // Treat identifier keys as strings
                    }
                    key = std::make_unique<LiteralExpr>(key_token);
                }
                else
                {
                    error_at_current("Expect dictionary key (identifier or string).");
                }
                
                consume(TokenType::Colon, "Expect ':' after dictionary key.");
                auto value = expression();
                
                elements.push_back({std::move(key), std::move(value)});
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RightBrace, "Expect '}' after dictionary.");
        return std::make_unique<DictExpr>(std::move(elements));
    }

    error_at_current("Expect expression.");
    return nullptr;
}

} // namespace blades
