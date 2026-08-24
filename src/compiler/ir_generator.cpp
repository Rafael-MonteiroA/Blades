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
        case TokenType::Ampersand:    emit(OpCode::BitAnd, line); break;
        case TokenType::Pipe:         emit(OpCode::BitOr, line); break;
        case TokenType::Caret:        emit(OpCode::BitXor, line); break;
        case TokenType::LessLess:     emit(OpCode::ShiftLeft, line); break;
        case TokenType::GreaterGreater: emit(OpCode::ShiftRight, line); break;
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
        case TokenType::Tilde: emit(OpCode::BitNot, line); break;
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
        int upvalue = resolve_upvalue(static_cast<int>(m_compiler_stack.size()) - 1, name);
        if (upvalue != -1) emit(OpCode::GetUpvalue, static_cast<u32>(upvalue), expr.name.span.start.line);
        else
        {
            u32 index = make_constant(Value(name));
            emit(OpCode::GetGlobal, index, expr.name.span.start.line);
        }
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
        int upvalue = resolve_upvalue(static_cast<int>(m_compiler_stack.size()) - 1, name);
        if (upvalue != -1) emit(OpCode::SetUpvalue, static_cast<u32>(upvalue), expr.name.span.start.line);
        else
        {
            u32 index = make_constant(Value(name));
            emit(OpCode::SetGlobal, index, expr.name.span.start.line);
        }
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

std::any IRGenerator::visit(const DictExpr& expr)
{
    for (const auto& [k, v] : expr.elements)
    {
        k->accept(*this);
        v->accept(*this);
    }
    emit(OpCode::BuildDict, static_cast<u32>(expr.elements.size()), 0);
    return std::any();
}

std::any IRGenerator::visit(const PropertyExpr& expr)
{
    expr.object->accept(*this);
    u32 name_idx = make_constant(Value(std::string(expr.name.lexeme)));
    emit(OpCode::GetProperty, name_idx, expr.name.span.start.line);
    return std::any();
}

std::any IRGenerator::visit(const PropertyAssignExpr& expr)
{
    expr.object->accept(*this);
    expr.value->accept(*this);
    u32 name_idx = make_constant(Value(std::string(expr.name.lexeme)));
    emit(OpCode::SetProperty, name_idx, expr.name.span.start.line);
    return std::any();
}

std::any IRGenerator::visit(const ThisExpr& expr)
{
    // 'this' is just a local variable implicitly placed at slot 0 of the method's frame.
    // However, our local resolution doesn't currently do string lookups in IRGenerator.
    // Wait, let's just use GetLocal with index 0.
    emit(OpCode::GetLocal, 0, expr.keyword.span.start.line);
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
        if (current()->locals.back().is_captured) {
            emit(OpCode::CloseUpvalue, 0, 0);
        } else {
            emit(OpCode::Pop, 0, 0); // Clean the stack
        }
        current()->locals.pop_back();
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
        if (current()->locals.back().is_captured) {
            emit(OpCode::CloseUpvalue, 0, 0);
        } else {
            emit(OpCode::Pop, 0, 0);
        }
        current()->locals.pop_back();
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
    func->captured_upvalues = current()->upvalues;
    m_compiler_stack.pop_back();
    
    // Now back to outer compiler context
    u32 func_idx = make_constant(Value(func));
    emit(OpCode::Closure, func_idx, decl.name.span.start.line);
    
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

std::any IRGenerator::visit(const FnExpr& expr)
{
    auto new_state = std::make_unique<CompilerState>();
    new_state->function = std::make_shared<ObjFunction>();
    new_state->function->name = "";
    new_state->function->arity = static_cast<u32>(expr.params.size());
    
    new_state->locals.push_back(Local{"", 0});
    new_state->scope_depth = 1;
    for (const auto& param : expr.params)
    {
        new_state->locals.push_back(Local{std::string(param.lexeme), 1});
    }
    
    m_compiler_stack.push_back(std::move(new_state));
    
    for (const auto& s : expr.body->statements)
    {
        s->accept(*this);
    }
    
    u32 idx = make_constant(Value(Nil{}));
    emit(OpCode::Constant, idx, 0);
    emit(OpCode::Return, 0); 
    
    auto func = current()->function;
    func->captured_upvalues = current()->upvalues;
    m_compiler_stack.pop_back();
    
    u32 func_idx = make_constant(Value(func));
    emit(OpCode::Closure, func_idx, 0);
    
    return std::any();
}

std::any IRGenerator::visit(const ClassDecl& decl)
{
    u32 name_idx = make_constant(Value(std::string(decl.name.lexeme)));
    
    // Instantiate the class definition at runtime
    emit(OpCode::Class, name_idx, decl.name.span.start.line);
    emit(OpCode::DefineGlobal, name_idx, decl.name.span.start.line);

    for (const auto& method : decl.methods)
    {
        auto comp = std::make_unique<CompilerState>();
        comp->function = std::make_shared<ObjFunction>();
        comp->function->name = std::string(method->name.lexeme);
        // The arity is params size. Note: 'this' is implicitly at slot 0 when called.
        comp->function->arity = static_cast<u32>(method->params.size());
        
        m_compiler_stack.push_back(std::move(comp));
        
        // Define parameters as locals
        // 'this' is local 0 implicitly. We can declare it to make it resolvable.
        current()->locals.push_back(Local{"this", current()->scope_depth});
        for (const auto& param : method->params)
        {
            current()->locals.push_back(Local{std::string(param.lexeme), current()->scope_depth});
        }
        
        method->body->accept(*this);
        
        // Implicit return nil if method doesn't return
        u32 nil_idx = make_constant(Value(Nil{}));
        emit(OpCode::Constant, nil_idx, decl.name.span.start.line);
        emit(OpCode::Return, 0, decl.name.span.start.line);
        
        auto method_func = m_compiler_stack.back()->function;
        method_func->captured_upvalues = m_compiler_stack.back()->upvalues;
        m_compiler_stack.pop_back();
        
        // Load the class back on top of the stack
        emit(OpCode::GetGlobal, name_idx, decl.name.span.start.line);
        
        u32 func_idx = make_constant(Value(method_func));
        emit(OpCode::Closure, func_idx, method->name.span.start.line);
        
        u32 method_name_idx = make_constant(Value(std::string(method->name.lexeme)));
        emit(OpCode::Method, method_name_idx, method->name.span.start.line);
    }
    
    return std::any();
}

std::any IRGenerator::visit(const ImportStmt& stmt)
{
    (void)stmt;
    return std::any();
}

std::any IRGenerator::visit(const YieldExpr& expr)
{
    if (expr.value)
    {
        expr.value->accept(*this);
    }
    else
    {
        u32 idx = make_constant(Value(Nil{}));
        emit(OpCode::Constant, idx, expr.keyword.span.start.line);
    }
    
    emit(OpCode::Yield, expr.keyword.span.start.line);
    return std::any();
}

std::any IRGenerator::visit(const MatchExpr& expr)
{
    expr.value->accept(*this);
    u32 line = expr.keyword.span.start.line;
    
    std::vector<u32> end_jumps;
    
    for (const auto& arm : expr.arms)
    {
        if (arm.pattern)
        {
            emit(OpCode::Dup, line);
            arm.pattern->accept(*this);
            emit(OpCode::Equal, line);
            
            u32 jump_if_false = emit_jump(OpCode::JumpIfFalse, line);
            
            // Match successful: pop the original match value and execute body
            emit(OpCode::Pop, line);
            arm.body->accept(*this);
            end_jumps.push_back(emit_jump(OpCode::Jump, line));
            
            // Patch for next arm
            patch_jump(jump_if_false);
        }
        else
        {
            // Default arm '_'
            emit(OpCode::Pop, line);
            arm.body->accept(*this);
            end_jumps.push_back(emit_jump(OpCode::Jump, line));
        }
    }
    
    for (u32 jump : end_jumps)
    {
        patch_jump(jump);
    }
    
    return std::any();
}

} // namespace blades
