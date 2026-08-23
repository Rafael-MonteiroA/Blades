#include "compiler/ir_generator.hpp"

namespace blades
{

IRChunk IRGenerator::generate(const std::vector<std::unique_ptr<Stmt>>& statements)
{
    for (const auto& stmt : statements)
    {
        stmt->accept(*this);
    }
    
    // Add implicit return at the end of the top-level script
    emit(OpCode::Return, 0);
    
    return std::move(m_chunk);
}

// ─────────────────────────────────────────────────────────────────────────────
// Expressions
// ─────────────────────────────────────────────────────────────────────────────

std::any IRGenerator::visit(const LiteralExpr& expr)
{
    Value val;
    if (expr.value.type == TokenType::Integer)
    {
        val = Value(std::stoi(std::string(expr.value.lexeme)));
    }
    else if (expr.value.type == TokenType::Float)
    {
        val = Value(std::stod(std::string(expr.value.lexeme)));
    }
    else if (expr.value.type == TokenType::True)
    {
        val = Value(true);
    }
    else if (expr.value.type == TokenType::False)
    {
        val = Value(false);
    }
    else if (expr.value.type == TokenType::String)
    {
        // Strip quotes for strings
        std::string s(expr.value.lexeme);
        if (s.length() >= 2 && s.front() == '"' && s.back() == '"')
        {
            s = s.substr(1, s.length() - 2);
        }
        val = Value(s);
    }
    else
    {
        val = Value(Nil{});
    }

    u32 index = make_constant(std::move(val));
    emit(OpCode::Constant, index, expr.value.span.start.line);
    return std::any();
}

std::any IRGenerator::visit(const BinaryExpr& expr)
{
    expr.left->accept(*this);
    expr.right->accept(*this);
    
    u32 line = expr.op.span.start.line;
    
    switch (expr.op.type)
    {
        case TokenType::Plus:         emit(OpCode::Add, line); break;
        case TokenType::Minus:        emit(OpCode::Subtract, line); break;
        case TokenType::Star:         emit(OpCode::Multiply, line); break;
        case TokenType::Slash:        emit(OpCode::Divide, line); break;
        case TokenType::EqualEqual:   emit(OpCode::Equal, line); break;
        case TokenType::BangEqual:    emit(OpCode::NotEqual, line); break;
        case TokenType::Greater:      emit(OpCode::Greater, line); break;
        case TokenType::GreaterEqual: emit(OpCode::GreaterEqual, line); break;
        case TokenType::Less:         emit(OpCode::Less, line); break;
        case TokenType::LessEqual:    emit(OpCode::LessEqual, line); break;
        case TokenType::And:          // Short-circuiting for AND/OR should be handled here, but we simplify for now
                                      break; 
        default: break; // Unreachable if semantic analysis passed
    }
    
    return std::any();
}

std::any IRGenerator::visit(const UnaryExpr& expr)
{
    expr.right->accept(*this);
    u32 line = expr.op.span.start.line;
    
    switch (expr.op.type)
    {
        case TokenType::Minus: emit(OpCode::Negate, line); break;
        case TokenType::Bang:  emit(OpCode::Not, line); break;
        default: break;
    }
    
    return std::any();
}

std::any IRGenerator::visit(const GroupingExpr& expr)
{
    expr.expression->accept(*this);
    return std::any();
}

std::any IRGenerator::visit(const VariableExpr& expr)
{
    std::string name(expr.name.lexeme);
    int arg = resolve_local(name);
    
    if (arg != -1)
    {
        emit(OpCode::GetLocal, static_cast<u32>(arg), expr.name.span.start.line);
    }
    else
    {
        u32 index = make_constant(Value(name));
        emit(OpCode::GetGlobal, index, expr.name.span.start.line);
    }
    
    return std::any();
}

std::any IRGenerator::visit(const AssignExpr& expr)
{
    expr.value->accept(*this);
    
    std::string name(expr.name.lexeme);
    int arg = resolve_local(name);
    
    if (arg != -1)
    {
        emit(OpCode::SetLocal, static_cast<u32>(arg), expr.name.span.start.line);
    }
    else
    {
        u32 index = make_constant(Value(name));
        emit(OpCode::SetGlobal, index, expr.name.span.start.line);
    }
    
    return std::any();
}

std::any IRGenerator::visit(const CallExpr& expr)
{
    expr.callee->accept(*this);
    for (const auto& arg : expr.arguments)
    {
        arg->accept(*this);
    }
    emit(OpCode::Call, static_cast<u32>(expr.arguments.size()), expr.paren.span.start.line);
    return std::any();
}

// ─────────────────────────────────────────────────────────────────────────────
// Statements
// ─────────────────────────────────────────────────────────────────────────────

std::any IRGenerator::visit(const ExprStmt& stmt)
{
    stmt.expression->accept(*this);
    // Normally we'd emit OP_POP here to discard the expression result, but we'll skip for this simple IR
    return std::any();
}

std::any IRGenerator::visit(const LetStmt& stmt)
{
    if (stmt.initializer)
    {
        stmt.initializer->accept(*this);
    }
    else
    {
        u32 idx = make_constant(Value(Nil{}));
        emit(OpCode::Constant, idx, stmt.name.span.start.line);
    }
    
    std::string name(stmt.name.lexeme);
    
    if (m_scope_depth > 0)
    {
        m_locals.push_back(Local{name, m_scope_depth});
    }
    else
    {
        u32 index = make_constant(Value(name));
        emit(OpCode::DefineGlobal, index, stmt.name.span.start.line);
    }
    
    return std::any();
}

std::any IRGenerator::visit(const BlockStmt& stmt)
{
    m_scope_depth++;
    
    for (const auto& s : stmt.statements)
    {
        s->accept(*this);
    }
    
    m_scope_depth--;
    
    // Pop locals
    while (!m_locals.empty() && m_locals.back().depth > m_scope_depth)
    {
        m_locals.pop_back();
        // Emitting OP_POP here to clean the stack would be proper
    }
    
    return std::any();
}

std::any IRGenerator::visit(const IfStmt& stmt)
{
    stmt.condition->accept(*this);
    
    // line info should ideally be from the 'if' token, but we don't have it in IfStmt. We'll use 0 or guess.
    u32 line = 0; 
    
    u32 then_jump = emit_jump(OpCode::JumpIfFalse, line);
    // Pop condition if false, skipping it for simplicity
    
    stmt.then_branch->accept(*this);
    
    u32 else_jump = emit_jump(OpCode::Jump, line);
    
    patch_jump(then_jump);
    
    if (stmt.else_branch)
    {
        stmt.else_branch->accept(*this);
    }
    
    patch_jump(else_jump);
    
    return std::any();
}

std::any IRGenerator::visit(const WhileStmt& stmt)
{
    u32 loop_start = static_cast<u32>(m_chunk.code.size());
    
    stmt.condition->accept(*this);
    
    u32 line = 0;
    u32 exit_jump = emit_jump(OpCode::JumpIfFalse, line);
    
    stmt.body->accept(*this);
    emit_loop(loop_start, line);
    
    patch_jump(exit_jump);
    
    return std::any();
}

std::any IRGenerator::visit(const ReturnStmt& stmt)
{
    if (stmt.value)
    {
        stmt.value->accept(*this);
    }
    else
    {
        u32 idx = make_constant(Value(Nil{}));
        emit(OpCode::Constant, idx, stmt.keyword.span.start.line);
    }
    
    emit(OpCode::Return, stmt.keyword.span.start.line);
    return std::any();
}

std::any IRGenerator::visit(const FunctionDecl& decl)
{
    (void)decl;
    // Skipping complex function declarations in this simple IR for now
    return std::any();
}

} // namespace blades
