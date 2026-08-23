#include "compiler/semantic_analyzer.hpp"

#include <iostream>
#include <sstream>

namespace blades
{

void SemanticAnalyzer::error(const Token& token, const std::string& message)
{
    std::ostringstream oss;
    oss << "[line " << token.span.start.line << "] Semantic Error";
    if (token.type == TokenType::Eof) {
        oss << " at end";
    } else {
        oss << " at '" << token.lexeme << "'";
    }
    oss << ": " << message;
    throw SemanticError(oss.str());
}

ValueType SemanticAnalyzer::evaluate(const Expr& expr)
{
    auto type = std::any_cast<ValueType>(expr.accept(*this));
    // Assign back to the AST node (mutable cast since we passed const ref, or we just trust the field is mutable)
    // Actually, `expr` is const. We can mark `resolved_type` as mutable in ast.hpp, or use const_cast.
    // We'll use const_cast for now to avoid polluting ast.hpp with mutables if not strictly needed.
    const_cast<Expr&>(expr).resolved_type = type;
    return type;
}

void SemanticAnalyzer::execute(const Stmt& stmt)
{
    stmt.accept(*this);
}

void SemanticAnalyzer::analyze(const std::vector<std::unique_ptr<Stmt>>& statements)
{
    // Ensure we have at least a global scope if it's completely empty
    m_symbols.declare("__init_global__", ValueType::Unknown);
    
    for (const auto& stmt : statements)
    {
        execute(*stmt);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Expressions
// ─────────────────────────────────────────────────────────────────────────────

std::any SemanticAnalyzer::visit(const LiteralExpr& expr)
{
    ValueType type = ValueType::Unknown;
    switch (expr.value.type)
    {
        case TokenType::Integer: type = ValueType::Int; break;
        case TokenType::Float:   type = ValueType::Float; break;
        case TokenType::String:  type = ValueType::String; break;
        case TokenType::True:
        case TokenType::False:   type = ValueType::Bool; break;
        default: break;
    }
    return type;
}

std::any SemanticAnalyzer::visit(const BinaryExpr& expr)
{
    ValueType left = evaluate(*expr.left);
    ValueType right = evaluate(*expr.right);
    
    if (left == ValueType::Unknown || right == ValueType::Unknown)
    {
        return ValueType::Unknown; // Cascade errors gracefully
    }
    
    switch (expr.op.type)
    {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
            if ((left == ValueType::Int || left == ValueType::Float) &&
                (right == ValueType::Int || right == ValueType::Float))
            {
                // If either is float, result is float, else int.
                return (left == ValueType::Float || right == ValueType::Float) ? ValueType::Float : ValueType::Int;
            }
            error(expr.op, "Operands must be numbers.");
            break;
            
        case TokenType::Greater:
        case TokenType::GreaterEqual:
        case TokenType::Less:
        case TokenType::LessEqual:
            if ((left == ValueType::Int || left == ValueType::Float) &&
                (right == ValueType::Int || right == ValueType::Float))
            {
                return ValueType::Bool;
            }
            error(expr.op, "Operands must be numbers.");
            break;
            
        case TokenType::EqualEqual:
        case TokenType::BangEqual:
            if (left != right)
            {
                error(expr.op, "Cannot compare different types.");
            }
            return ValueType::Bool;
            
        case TokenType::And:
        case TokenType::Or:
            if (left == ValueType::Bool && right == ValueType::Bool)
            {
                return ValueType::Bool;
            }
            error(expr.op, "Operands must be booleans.");
            break;
            
        default:
            break;
    }
    
    return ValueType::Unknown;
}

std::any SemanticAnalyzer::visit(const UnaryExpr& expr)
{
    ValueType right = evaluate(*expr.right);
    if (right == ValueType::Unknown) return ValueType::Unknown;
    
    switch (expr.op.type)
    {
        case TokenType::Minus:
            if (right != ValueType::Int && right != ValueType::Float)
            {
                error(expr.op, "Operand must be a number.");
            }
            return right;
            
        case TokenType::Bang:
            if (right != ValueType::Bool)
            {
                error(expr.op, "Operand must be a boolean.");
            }
            return ValueType::Bool;
            
        default:
            break;
    }
    return ValueType::Unknown;
}

std::any SemanticAnalyzer::visit(const GroupingExpr& expr)
{
    return evaluate(*expr.expression);
}

std::any SemanticAnalyzer::visit(const VariableExpr& expr)
{
    auto type = m_symbols.lookup(expr.name.lexeme);
    if (!type.has_value())
    {
        error(expr.name, "Undefined variable.");
        return ValueType::Unknown;
    }
    return type.value();
}

std::any SemanticAnalyzer::visit(const AssignExpr& expr)
{
    ValueType value_type = evaluate(*expr.value);
    
    auto var_type = m_symbols.lookup(expr.name.lexeme);
    if (!var_type.has_value())
    {
        error(expr.name, "Undefined variable.");
        return value_type;
    }
    
    if (var_type.value() != value_type && value_type != ValueType::Unknown)
    {
        error(expr.name, "Cannot assign value of type " + std::string(to_string(value_type)) + 
                         " to variable of type " + std::string(to_string(var_type.value())) + ".");
    }
    
    return value_type;
}

std::any SemanticAnalyzer::visit(const CallExpr& expr)
{
    // For now, we just evaluate the callee and args.
    // Proper function type checking requires a Function type in ValueType.
    evaluate(*expr.callee);
    for (const auto& arg : expr.arguments)
    {
        evaluate(*arg);
    }
    return ValueType::Unknown; // Placeholder until we have Function types
}

// ─────────────────────────────────────────────────────────────────────────────
// Statements
// ─────────────────────────────────────────────────────────────────────────────

std::any SemanticAnalyzer::visit(const ExprStmt& stmt)
{
    evaluate(*stmt.expression);
    return std::any();
}

std::any SemanticAnalyzer::visit(const LetStmt& stmt)
{
    ValueType type = ValueType::Unknown;
    if (stmt.initializer)
    {
        type = evaluate(*stmt.initializer);
    }
    m_symbols.declare(stmt.name.lexeme, type);
    return std::any();
}

std::any SemanticAnalyzer::visit(const BlockStmt& stmt)
{
    m_symbols.begin_scope();
    for (const auto& s : stmt.statements)
    {
        execute(*s);
    }
    m_symbols.end_scope();
    return std::any();
}

std::any SemanticAnalyzer::visit(const IfStmt& stmt)
{
    ValueType cond = evaluate(*stmt.condition);
    if (cond != ValueType::Bool && cond != ValueType::Unknown)
    {
        // We don't have a token for 'if' easily available here, so we throw generic or we could pass the token.
        // For simplicity, we throw a runtime error.
        throw SemanticError("Condition in 'if' statement must be a boolean.");
    }
    
    execute(*stmt.then_branch);
    if (stmt.else_branch)
    {
        execute(*stmt.else_branch);
    }
    return std::any();
}

std::any SemanticAnalyzer::visit(const WhileStmt& stmt)
{
    ValueType cond = evaluate(*stmt.condition);
    if (cond != ValueType::Bool && cond != ValueType::Unknown)
    {
        throw SemanticError("Condition in 'while' statement must be a boolean.");
    }
    execute(*stmt.body);
    return std::any();
}

std::any SemanticAnalyzer::visit(const ReturnStmt& stmt)
{
    if (stmt.value)
    {
        evaluate(*stmt.value);
    }
    return std::any();
}

std::any SemanticAnalyzer::visit(const FunctionDecl& decl)
{
    // Simple mock: declare function name
    m_symbols.declare(decl.name.lexeme, ValueType::Unknown);
    
    m_symbols.begin_scope();
    for (const auto& param : decl.params)
    {
        m_symbols.declare(param.lexeme, ValueType::Unknown);
    }
    
    // Execute body directly (since BlockStmt handles its own scope, we might have double scope, 
    // but BlockStmt visit is fine). Actually BlockStmt visit will create a new scope.
    execute(*decl.body);
    
    m_symbols.end_scope();
    return std::any();
}

} // namespace blades
