#include "compiler/ir_generator.hpp"

namespace blades
{

std::shared_ptr<ObjFunction> IRGenerator::generate(const std::vector<std::unique_ptr<Stmt>>& statements)
{
    auto top_level = std::make_unique<CompilerState>();
    top_level->function = std::make_shared<ObjFunction>();
    top_level->function->name = ""; // Script top-level
    top_level->function->arity = 0;
    
    // Local 0 is reserved for the function itself
    top_level->locals.push_back(Local{"", 0});
    
    m_compiler_stack.push_back(std::move(top_level));

    for (const auto& stmt : statements)
    {
        stmt->accept(*this);
    }
    
    // Add implicit return at the end of the top-level script
    emit(OpCode::Return, 0);
    
    auto func = current()->function;
    m_compiler_stack.pop_back();
    return func;
}

// ─────────────────────────────────────────────────────────────────────────────
// Expressions
// ─────────────────────────────────────────────────────────────────────────────

std::any IRGenerator::visit(const LiteralExpr& expr)
{
    Value val;
    if (expr.value.type == TokenType::Integer) val = Value(std::stoi(std::string(expr.value.lexeme)));
    else if (expr.value.type == TokenType::Float) val = Value(std::stod(std::string(expr.value.lexeme)));
    else if (expr.value.type == TokenType::True) val = Value(true);
    else if (expr.value.type == TokenType::False) val = Value(false);
    else if (expr.value.type == TokenType::String)
    {
        std::string s(expr.value.lexeme);
        if (s.length() >= 2 && s.front() == '"' && s.back() == '"') s = s.substr(1, s.length() - 2);
        val = Value(s);
    }
    else val = Value(Nil{});

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
        default: break;
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
    
    if (arg != -1) emit(OpCode::GetLocal, static_cast<u32>(arg), expr.name.span.start.line);
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
    
    if (arg != -1) emit(OpCode::SetLocal, static_cast<u32>(arg), expr.name.span.start.line);
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

std::any IRGenerator::visit(const ArrayExpr& expr)
{
    for (const auto& el : expr.elements)
    {
        el->accept(*this);
    }
    emit(OpCode::BuildList, static_cast<u32>(expr.elements.size()), 0);
    return std::any();
}

std::any IRGenerator::visit(const SubscriptExpr& expr)
{
    expr.object->accept(*this);
    expr.index->accept(*this);
    emit(OpCode::GetSubscript, 0, 0);
    return std::any();
}

std::any IRGenerator::visit(const SubscriptAssignExpr& expr)
{
    expr.object->accept(*this);
    expr.index->accept(*this);
    expr.value->accept(*this);
    emit(OpCode::SetSubscript, 0, 0);
    return std::any();
}

// ─────────────────────────────────────────────────────────────────────────────
// Statements
// ─────────────────────────────────────────────────────────────────────────────

std::any IRGenerator::visit(const ExprStmt& stmt)
{
    stmt.expression->accept(*this);
    emit(OpCode::Pop, 0, 0); // Pop the expression result
    return std::any();
}

std::any IRGenerator::visit(const LetStmt& stmt)
{
    if (stmt.initializer) stmt.initializer->accept(*this);
    else
    {
        u32 idx = make_constant(Value(Nil{}));
        emit(OpCode::Constant, idx, stmt.name.span.start.line);
    }
    
    std::string name(stmt.name.lexeme);
    
    if (current()->scope_depth > 0)
    {
        current()->locals.push_back(Local{name, current()->scope_depth});
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
    current()->scope_depth++;
    for (const auto& s : stmt.statements) s->accept(*this);
    current()->scope_depth--;
    
    // Pop locals
    while (!current()->locals.empty() && current()->locals.back().depth > current()->scope_depth)
    {
        current()->locals.pop_back();
        emit(OpCode::Pop, 0, 0); // Clean the stack
    }
    return std::any();
}

std::any IRGenerator::visit(const IfStmt& stmt)
{
    stmt.condition->accept(*this);
    u32 line = 0; 
    
    u32 then_jump = emit_jump(OpCode::JumpIfFalse, line);
    stmt.then_branch->accept(*this);
    
    u32 else_jump = emit_jump(OpCode::Jump, line);
    patch_jump(then_jump);
    
    if (stmt.else_branch) stmt.else_branch->accept(*this);
    patch_jump(else_jump);
    
    return std::any();
}

std::any IRGenerator::visit(const WhileStmt& stmt)
{
    u32 loop_start = static_cast<u32>(current_chunk()->code.size());
    stmt.condition->accept(*this);
    
    u32 line = 0;
    u32 exit_jump = emit_jump(OpCode::JumpIfFalse, line);
    
    stmt.body->accept(*this);
    emit_loop(loop_start, line);
    patch_jump(exit_jump);
    
    return std::any();
}

std::any IRGenerator::visit(const ForStmt& stmt)
{
    current()->scope_depth++;
    
    if (stmt.initializer) stmt.initializer->accept(*this);
    
    u32 loop_start = static_cast<u32>(current_chunk()->code.size());
    
    u32 exit_jump = 0;
    if (stmt.condition)
    {
        stmt.condition->accept(*this);
        exit_jump = emit_jump(OpCode::JumpIfFalse, 0);
    }
    
    stmt.body->accept(*this);
    
    if (stmt.increment)
    {
        stmt.increment->accept(*this);
        emit(OpCode::Pop, 0, 0); // Pop expression result
    }
    
    emit_loop(loop_start, 0);
    
    if (stmt.condition)
    {
        patch_jump(exit_jump);
    }
    
    current()->scope_depth--;
    while (!current()->locals.empty() && current()->locals.back().depth > current()->scope_depth)
    {
        current()->locals.pop_back();
        emit(OpCode::Pop, 0, 0);
    }
    
    return std::any();
}

std::any IRGenerator::visit(const ReturnStmt& stmt)
{
    if (stmt.value) stmt.value->accept(*this);
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
    auto new_state = std::make_unique<CompilerState>();
    new_state->function = std::make_shared<ObjFunction>();
    new_state->function->name = std::string(decl.name.lexeme);
    new_state->function->arity = static_cast<u32>(decl.params.size());
    
    // Local 0 is reserved for the function itself
    new_state->locals.push_back(Local{new_state->function->name, 0});
    
    // Declare params as locals (depth 1)
    new_state->scope_depth = 1;
    for (const auto& param : decl.params)
    {
        new_state->locals.push_back(Local{std::string(param.lexeme), 1});
    }
    
    m_compiler_stack.push_back(std::move(new_state));
    
    // Compile body
    for (const auto& s : decl.body->statements)
    {
        s->accept(*this);
    }
    
    // Implicit return nil
    u32 idx = make_constant(Value(Nil{}));
    emit(OpCode::Constant, idx, 0);
    emit(OpCode::Return, 0); 
    
    auto func = current()->function;
    m_compiler_stack.pop_back();
    
    // Now back to outer compiler context
    u32 func_idx = make_constant(Value(func));
    emit(OpCode::Constant, func_idx, decl.name.span.start.line);
    
    if (current()->scope_depth > 0)
    {
        current()->locals.push_back(Local{std::string(decl.name.lexeme), current()->scope_depth});
    }
    else
    {
        u32 name_idx = make_constant(Value(std::string(decl.name.lexeme)));
        emit(OpCode::DefineGlobal, name_idx, decl.name.span.start.line);
    }
    
    return std::any();
}

} // namespace blades
