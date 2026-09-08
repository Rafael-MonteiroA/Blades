#include "compiler/semantic_analyzer.hpp"

#include <iostream>
#include <sstream>

namespace blades
{

void SemanticAnalyzer::error(const Token& token, const std::string& message)
{
    std::ostringstream oss;
    oss << "[" << token.span.start.to_string() << "] Semantic Error";
    if (token.type == TokenType::Eof) {
        oss << " at end";
    } else {
        oss << " at '" << token.lexeme << "'";
    }
    oss << ": " << message;
    throw SemanticError(oss.str());
}

ValueType SemanticAnalyzer::type_from_annotation(const std::optional<Token>& annotation)
{
    if (!annotation.has_value()) return ValueType::Unknown;

    const std::string name(annotation->lexeme);
    if (name == "int" || name == "i32" || name == "i64") return ValueType::Int;
    if (name == "float" || name == "f32" || name == "f64" || name == "double") return ValueType::Float;
    if (name == "number") return ValueType::Number;
    if (name == "bool") return ValueType::Bool;
    if (name == "string" || name == "str") return ValueType::String;
    if (name == "nil" || name == "unit" || name == "void") return ValueType::Nil;
    if (name == "any") return ValueType::Any;
    if (name == "array" || name == "list") return ValueType::Array;
    if (name == "dict" || name == "map") return ValueType::Dict;

    error(*annotation, "Unknown type '" + name + "'.");
    return ValueType::Unknown;
}

bool SemanticAnalyzer::compatible(ValueType expected, ValueType actual) const
{
    if (expected == ValueType::Number)
    {
        return actual == ValueType::Int || actual == ValueType::Float ||
               actual == ValueType::Unknown || actual == ValueType::Any;
    }
    return expected == ValueType::Unknown || actual == ValueType::Unknown ||
           expected == ValueType::Any || actual == ValueType::Any || expected == actual;
}

const SemanticAnalyzer::FunctionSignature* SemanticAnalyzer::find_function(std::string_view name) const
{
    auto local = m_functions.find(std::string(name));
    if (local != m_functions.end()) return &local->second;

    if (m_known_functions != nullptr)
    {
        auto known = m_known_functions->find(std::string(name));
        if (known != m_known_functions->end()) return &known->second;
    }
    return nullptr;
}

ValueType SemanticAnalyzer::evaluate(const Expr& expr)
{
    auto type = std::any_cast<ValueType>(expr.accept(*this));
    expr.resolved_type = type;
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

    // Collect signatures before walking bodies so forward calls and recursion
    // receive the same checks as calls to functions declared earlier.
    for (const auto& stmt : statements)
    {
        if (const auto* decl = dynamic_cast<const FunctionDecl*>(stmt.get()))
        {
            FunctionSignature signature;
            for (const auto& parameter : decl->params)
                signature.parameters.push_back(type_from_annotation(parameter.type));
            signature.return_type = type_from_annotation(decl->return_type);
            m_functions[std::string(decl->name.lexeme)] = std::move(signature);
        }
    }
    
    for (const auto& stmt : statements)
    {
        execute(*stmt);
    }

    // Persist signatures only after the complete unit succeeds. This keeps a
    // failed REPL entry from registering half-checked functions.
    if (m_known_functions != nullptr)
    {
        for (const auto& [name, signature] : m_functions)
            (*m_known_functions)[name] = signature;
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
        case TokenType::Nil:     type = ValueType::Nil; break;
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
            if ((left == ValueType::Int || left == ValueType::Float) &&
                (right == ValueType::Int || right == ValueType::Float))
            {
                return (left == ValueType::Float || right == ValueType::Float) ? ValueType::Float : ValueType::Int;
            }
            if (left == ValueType::String || right == ValueType::String) return ValueType::String;
            error(expr.op, "Operands must be numbers or at least one must be a string.");
            break;
            
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
            if ((left == ValueType::Int || left == ValueType::Float) &&
                (right == ValueType::Int || right == ValueType::Float))
            {
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
        case TokenType::AmpAmp:
        case TokenType::Or:
        case TokenType::PipePipe:
            // Short-circuit operators — both operands should be truthy/falsy values.
            // We accept Any/Unknown permissively (dynamic types from functions etc.)
            if ((left == ValueType::Bool || left == ValueType::Unknown || left == ValueType::Any) &&
                (right == ValueType::Bool || right == ValueType::Unknown || right == ValueType::Any))
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

    if (!m_symbols.is_mutable(expr.name.lexeme))
    {
        error(expr.name, "Cannot assign to a constant binding.");
    }
    
    if (var_type.value() != value_type && var_type.value() != ValueType::Unknown && value_type != ValueType::Unknown)
    {
        error(expr.name, "Cannot assign value of type " + std::string(to_string(value_type)) + 
                         " to variable of type " + std::string(to_string(var_type.value())) + ".");
    }
    
    return value_type;
}

std::any SemanticAnalyzer::visit(const CallExpr& expr)
{
    evaluate(*expr.callee);
    std::vector<ValueType> argument_types;
    for (const auto& arg : expr.arguments)
    {
        argument_types.push_back(evaluate(*arg));
    }

    if (const auto* variable = dynamic_cast<const VariableExpr*>(expr.callee.get()))
    {
        const auto* signature = find_function(variable->name.lexeme);
        if (signature != nullptr)
        {
            if (signature->parameters.size() != argument_types.size())
            {
                error(expr.paren, "Expected " + std::to_string(signature->parameters.size()) +
                                 " argument(s), got " + std::to_string(argument_types.size()) + ".");
            }
            for (size_t i = 0; i < argument_types.size(); ++i)
            {
                if (!compatible(signature->parameters[i], argument_types[i]))
                {
                    error(expr.paren, "Argument " + std::to_string(i + 1) + " has type " +
                                     std::string(to_string(argument_types[i])) + ", expected " +
                                     std::string(to_string(signature->parameters[i])) + ".");
                }
            }
            return signature->return_type;
        }
    }
    return ValueType::Unknown; 
}

std::any SemanticAnalyzer::visit(const ArrayExpr& expr)
{
    ValueType element_type = ValueType::Unknown;
    for (const auto& el : expr.elements)
    {
        ValueType current_type = evaluate(*el);
        if (element_type == ValueType::Unknown) element_type = current_type;
        else if (!compatible(element_type, current_type)) element_type = ValueType::Any;
    }
    return ValueType::Array;
}

std::any SemanticAnalyzer::visit(const SubscriptExpr& expr)
{
    evaluate(*expr.object);
    ValueType idx_type = evaluate(*expr.index);
    if (idx_type != ValueType::Int && idx_type != ValueType::String && idx_type != ValueType::Unknown)
    {
        throw SemanticError("Index must be an integer or string.");
    }
    return ValueType::Unknown;
}

std::any SemanticAnalyzer::visit(const SubscriptAssignExpr& expr)
{
    evaluate(*expr.object);
    evaluate(*expr.index); // Index can be int for arrays, or string for dicts. So we just evaluate.
    ValueType val_type = evaluate(*expr.value);
    return val_type;
}

std::any SemanticAnalyzer::visit(const DictExpr& expr)
{
    for (const auto& [k, v] : expr.elements)
    {
        evaluate(*k);
        evaluate(*v);
    }
    return ValueType::Unknown;
}

std::any SemanticAnalyzer::visit(const PropertyExpr& expr)
{
    evaluate(*expr.object);
    return ValueType::Unknown;
}

std::any SemanticAnalyzer::visit(const PropertyAssignExpr& expr)
{
    evaluate(*expr.object);
    ValueType val_type = evaluate(*expr.value);
    return val_type;
}

std::any SemanticAnalyzer::visit(const ThisExpr& expr)
{
    (void)expr;
    // Technically we should check if we are inside a method.
    // For now we just return Unknown.
    return ValueType::Unknown;
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
    const ValueType annotated_type = type_from_annotation(stmt.type);
    if (stmt.initializer)
    {
        type = evaluate(*stmt.initializer);
    }
    if (stmt.type)
    {
        if (type != ValueType::Unknown && !compatible(annotated_type, type))
        {
            error(*stmt.type, "Variable '" + std::string(stmt.name.lexeme) + "' is " +
                              std::string(to_string(type)) + ", expected " +
                              std::string(to_string(annotated_type)) + ".");
        }
        type = annotated_type;
    }
    if (stmt.is_const && !stmt.initializer)
    {
        error(stmt.name, "A constant must have an initializer.");
    }
    m_symbols.declare(stmt.name.lexeme, type, !stmt.is_const);
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
    ValueType cond_type = evaluate(*stmt.condition);
    if (cond_type != ValueType::Bool && cond_type != ValueType::Unknown)
    {
        throw SemanticError("Loop condition must be a boolean.");
    }
    execute(*stmt.body);
    return std::any();
}

std::any SemanticAnalyzer::visit(const ForStmt& stmt)
{
    m_symbols.begin_scope();
    
    if (stmt.initializer) execute(*stmt.initializer);
    
    if (stmt.condition)
    {
        ValueType cond_type = evaluate(*stmt.condition);
        if (cond_type != ValueType::Bool && cond_type != ValueType::Unknown)
        {
            throw SemanticError("Loop condition must be a boolean.");
        }
    }
    
    if (stmt.increment) evaluate(*stmt.increment);
    
    execute(*stmt.body);
    
    m_symbols.end_scope();
    return std::any();
}

std::any SemanticAnalyzer::visit(const ReturnStmt& stmt)
{
    if (m_return_types.empty())
    {
        error(stmt.keyword, "Cannot return from the top level.");
    }

    ValueType actual = ValueType::Nil;
    if (stmt.value)
    {
        actual = evaluate(*stmt.value);
    }
    if (!compatible(m_return_types.back(), actual))
    {
        error(stmt.keyword, "Return type is " + std::string(to_string(actual)) +
                           ", expected " + std::string(to_string(m_return_types.back())) + ".");
    }
    return std::any();
}

std::any SemanticAnalyzer::visit(const FunctionDecl& decl)
{
    m_symbols.declare(decl.name.lexeme, ValueType::Function);
    
    m_symbols.begin_scope();
    for (const auto& param : decl.params)
    {
        m_symbols.declare(param.name.lexeme, type_from_annotation(param.type));
    }

    const auto* signature = find_function(decl.name.lexeme);
    m_return_types.push_back(signature == nullptr ? ValueType::Unknown : signature->return_type);
    
    // Execute body directly (since BlockStmt handles its own scope, we might have double scope, 
    // but BlockStmt visit is fine). Actually BlockStmt visit will create a new scope.
    execute(*decl.body);
    
    m_symbols.end_scope();
    m_return_types.pop_back();
    return std::any();
}

std::any SemanticAnalyzer::visit(const ClassDecl& decl)
{
    m_symbols.declare(decl.name.lexeme, ValueType::Unknown);
    
    if (decl.superclass) {
        if (decl.name.lexeme == decl.superclass->name.lexeme) {
            error(decl.superclass->name, "A class cannot inherit from itself.");
        }
        decl.superclass->accept(*this);
        m_symbols.begin_scope();
        m_symbols.declare("super", ValueType::Unknown);
    }
    
    m_symbols.begin_scope();
    m_symbols.declare("this", ValueType::Unknown);
    
    for (const auto& method : decl.methods)
    {
        m_symbols.begin_scope();
        m_return_types.push_back(type_from_annotation(method->return_type));
        for (const auto& param : method->params)
        {
            m_symbols.declare(param.name.lexeme, type_from_annotation(param.type));
        }
        
        execute(*method->body);
        m_symbols.end_scope();
        m_return_types.pop_back();
    }
    
    m_symbols.end_scope();
    
    if (decl.superclass) {
        m_symbols.end_scope();
    }
    
    return std::any();
}

std::any SemanticAnalyzer::visit(const SuperExpr& expr)
{
    m_symbols.lookup(expr.keyword.lexeme);
    return std::any(ValueType::Unknown);
}

std::any SemanticAnalyzer::visit(const FnExpr& expr)
{
    m_symbols.begin_scope();
    for (const auto& param : expr.params)
    {
        m_symbols.declare(param.name.lexeme, type_from_annotation(param.type));
    }

    m_return_types.push_back(ValueType::Unknown);
    
    execute(*expr.body);
    m_symbols.end_scope();
    m_return_types.pop_back();
    
    return ValueType::Unknown;
}

std::any SemanticAnalyzer::visit(const YieldExpr& expr)
{
    if (expr.value)
    {
        evaluate(*expr.value);
    }
    return ValueType::Unknown;
}

std::any SemanticAnalyzer::visit(const MatchExpr& expr)
{
    evaluate(*expr.value);
    
    bool has_default = false;
    for (const auto& arm : expr.arms)
    {
        if (arm.is_default())
        {
            has_default = true;
        }
        else
        {
            // Validate all patterns
            for (const auto& pat : arm.patterns)
            {
                evaluate(*pat);
            }
        }
        
        // Validate guard if present
        if (arm.guard)
        {
            evaluate(*arm.guard);
        }
        
        // Validate body
        if (arm.body_block)
        {
            execute(*arm.body_block);
        }
        else if (arm.body_expr)
        {
            evaluate(*arm.body_expr);
        }
    }
    
    // Note: not requiring a default arm — match can return nil if no arm matches
    (void)has_default;
    
    return ValueType::Unknown;
}

std::any SemanticAnalyzer::visit(const ImportStmt& stmt)
{
    (void)stmt;
    return std::any();
}

std::any SemanticAnalyzer::visit(const BreakStmt& stmt)
{
    (void)stmt;
    return std::any();
}

std::any SemanticAnalyzer::visit(const ContinueStmt& stmt)
{
    (void)stmt;
    return std::any();
}

} // namespace blades
