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
    if (expr.value.type == TokenType::Integer)
    {
        emit_constant(Value(static_cast<int64_t>(std::stoll(std::string(expr.value.lexeme)))), expr.value.span.start.line);
    }
    else if (expr.value.type == TokenType::Float)
    {
        emit_constant(Value(std::stod(std::string(expr.value.lexeme))), expr.value.span.start.line);
    }
    else if (expr.value.type == TokenType::True)
    {
        emit_constant(Value(true), expr.value.span.start.line);
    }
    else if (expr.value.type == TokenType::False)
    {
        emit_constant(Value(false), expr.value.span.start.line);
    }
    else if (expr.value.type == TokenType::Nil)
    {
        emit_constant(Value(Nil{}), expr.value.span.start.line);
    }
    else if (expr.value.type == TokenType::String)
    {
        std::string s(expr.value.lexeme);
        // Strip surrounding quotes
        if (s.length() >= 2 && s.front() == '"' && s.back() == '"')
            s = s.substr(1, s.length() - 2);
        // Process escape sequences
        std::string result;
        result.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '\\' && i + 1 < s.size())
            {
                switch (s[i + 1])
                {
                    case 'n':  result += '\n'; ++i; break;
                    case 't':  result += '\t'; ++i; break;
                    case 'r':  result += '\r'; ++i; break;
                    case '\\': result += '\\'; ++i; break;
                    case '"':  result += '"';  ++i; break;
                    case '0':  result += '\0'; ++i; break;
                    default:
                        result += s[i]; // keep backslash for unknown sequences
                        break;
                }
            }
            else
            {
                result += s[i];
            }
        }
        emit_constant(Value(std::move(result)), expr.value.span.start.line);
    }
    return std::any();
}

std::any IRGenerator::visit(const BinaryExpr& expr)
{
    u32 line = expr.op.span.start.line;

    // Short-circuit: `and` / `&&`
    // Semantics: evaluate left; if falsy, result is left (falsy); else result is right.
    // JumpIfFalse always pops, so we Dup left first to preserve it on the short-circuit path.
    if (expr.op.type == TokenType::And || expr.op.type == TokenType::AmpAmp)
    {
        expr.left->accept(*this);
        emit(OpCode::Dup, line);           // stack: [left, left]
        u32 false_jump = emit_jump(OpCode::JumpIfFalse, line); // pops top copy; if falsy jumps
        // Left was truthy: the duplicate was popped by JumpIfFalse, original left remains
        emit(OpCode::Pop, line);           // pop original left, will be replaced by right
        expr.right->accept(*this);         // stack: [right]
        patch_jump(false_jump);            // falsy path lands here with original left still on stack
        return std::any();
    }

    // Short-circuit: `or` / `||`
    // Semantics: evaluate left; if truthy, result is left (truthy); else result is right.
    if (expr.op.type == TokenType::Or || expr.op.type == TokenType::PipePipe)
    {
        expr.left->accept(*this);
        emit(OpCode::Dup, line);           // stack: [left, left]
        u32 false_jump = emit_jump(OpCode::JumpIfFalse, line); // pops top; if falsy jumps to else
        // Left was truthy: duplicate popped, original left is result — jump to end
        u32 end_jump = emit_jump(OpCode::Jump, line);
        patch_jump(false_jump);            // left was falsy: original left still on stack
        emit(OpCode::Pop, line);           // pop the falsy left
        expr.right->accept(*this);         // stack: [right]
        patch_jump(end_jump);
        return std::any();
    }

    expr.left->accept(*this);
    expr.right->accept(*this);

    switch (expr.op.type)
    {
        case TokenType::Plus:           emit(OpCode::Add, line); break;
        case TokenType::Minus:          emit(OpCode::Subtract, line); break;
        case TokenType::Star:           emit(OpCode::Multiply, line); break;
        case TokenType::Slash:          emit(OpCode::Divide, line); break;
        case TokenType::Percent:        emit(OpCode::Modulo, line); break;
        case TokenType::EqualEqual:     emit(OpCode::Equal, line); break;
        case TokenType::BangEqual:      emit(OpCode::NotEqual, line); break;
        case TokenType::Greater:        emit(OpCode::Greater, line); break;
        case TokenType::GreaterEqual:   emit(OpCode::GreaterEqual, line); break;
        case TokenType::Less:           emit(OpCode::Less, line); break;
        case TokenType::LessEqual:      emit(OpCode::LessEqual, line); break;
        case TokenType::Ampersand:      emit(OpCode::BitAnd, line); break;
        case TokenType::Pipe:           emit(OpCode::BitOr, line); break;
        case TokenType::Caret:          emit(OpCode::BitXor, line); break;
        case TokenType::LessLess:       emit(OpCode::ShiftLeft, line); break;
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

    // Push loop context so break/continue know where they are
    m_loop_stack.push_back(LoopContext{loop_start, {}, current()->scope_depth});

    stmt.body->accept(*this);
    emit_loop(loop_start, line);
    patch_jump(exit_jump);

    // Patch all break jumps to land here (after exit_jump patch)
    LoopContext ctx = std::move(m_loop_stack.back());
    m_loop_stack.pop_back();
    for (u32 jump_idx : ctx.break_jumps)
    {
        patch_jump(jump_idx);
    }

    return std::any();
}

std::any IRGenerator::visit(const ForStmt& stmt)
{
    current()->scope_depth++;

    if (stmt.initializer) stmt.initializer->accept(*this);

    u32 loop_start = static_cast<u32>(current_chunk()->code.size());

    u32 exit_jump = 0;
    bool has_condition = (stmt.condition != nullptr);
    if (has_condition)
    {
        stmt.condition->accept(*this);
        exit_jump = emit_jump(OpCode::JumpIfFalse, 0);
    }

    // Body is compiled inside loop context; continue should jump to increment
    // We'll record the body start first, then patch continue offsets after body.
    // For simplicity: push context with loop_start pointing to the condition
    // (continue will jump back to condition recheck, then increment runs via Loop).
    // A cleaner approach: emit body, then increment, then Loop.
    // We jump past the increment if needed, execute body, fall through to increment.

    // Jump past increment to body
    u32 body_jump = emit_jump(OpCode::Jump, 0);

    // Increment section — continue lands here
    u32 increment_start = static_cast<u32>(current_chunk()->code.size());
    if (stmt.increment)
    {
        stmt.increment->accept(*this);
        emit(OpCode::Pop, 0, 0);
    }
    emit_loop(loop_start, 0);

    // Body section
    patch_jump(body_jump);

    m_loop_stack.push_back(LoopContext{increment_start, {}, current()->scope_depth});

    stmt.body->accept(*this);

    // After body, jump to increment
    emit_loop(increment_start, 0);

    // Patch exit
    if (has_condition)
    {
        patch_jump(exit_jump);
    }

    // Patch break jumps
    LoopContext ctx = std::move(m_loop_stack.back());
    m_loop_stack.pop_back();
    for (u32 jump_idx : ctx.break_jumps)
    {
        patch_jump(jump_idx);
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
    
    emit(OpCode::Class, name_idx, decl.name.span.start.line);
    emit(OpCode::DefineGlobal, name_idx, decl.name.span.start.line);

    if (decl.superclass) {
        decl.superclass->accept(*this);
        
        current()->scope_depth++;
        current()->locals.push_back(Local{"super", current()->scope_depth});
        
        emit(OpCode::GetLocal, static_cast<u32>(current()->locals.size() - 1), decl.name.span.start.line);
        emit(OpCode::GetGlobal, name_idx, decl.name.span.start.line);
        emit(OpCode::Inherit, 0, decl.name.span.start.line);
    }

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
    
    if (decl.superclass) {
        emit(OpCode::CloseUpvalue, 0, decl.name.span.start.line);
        current()->locals.pop_back();
        current()->scope_depth--;
    }
    
    return std::any();
}

std::any IRGenerator::visit(const SuperExpr& expr)
{
    auto emit_var = [&](const std::string& name) {
        int arg = resolve_local(name);
        if (arg != -1) emit(OpCode::GetLocal, static_cast<u32>(arg), expr.keyword.span.start.line);
        else {
            int upvalue = resolve_upvalue(static_cast<int>(m_compiler_stack.size()) - 1, name);
            if (upvalue != -1) emit(OpCode::GetUpvalue, static_cast<u32>(upvalue), expr.keyword.span.start.line);
            else {
                u32 index = make_constant(Value(name));
                emit(OpCode::GetGlobal, index, expr.keyword.span.start.line);
            }
        }
    };
    
    emit_var("this"); // push 'this'
    emit_var("super"); // push 'super' class
    
    u32 name_idx = make_constant(Value(std::string(expr.method.lexeme)));
    emit(OpCode::GetSuper, name_idx, expr.keyword.span.start.line);
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
    // Strategy:
    //   1. Evaluate match value — stays on stack throughout.
    //   2. For each arm:
    //      a. Non-default: test or-patterns. On any match -> pop match val, run body, push result, jump to end.
    //         On ALL patterns miss -> fall through to next arm.
    //      b. Default (_) arm: pop match value, run body, push result, jump to end.
    //   3. If no arm matched: pop match value, push nil.
    //   4. All end_jumps land AFTER the nil push (so there's always 1 result on stack).
    //
    // IMPORTANT: body_block arms do NOT push a value; we push nil for them.
    //            body_expr arms push their expression result.

    expr.value->accept(*this);
    u32 line = expr.keyword.span.start.line;
    
    // Pre-compute the nil constant index once
    u32 nil_idx = make_constant(Value(Nil{}));

    // end_jumps: all Jump instructions that need to be patched to after the whole match.
    std::vector<u32> end_jumps;
    
    // Helper lambda: emit body and ensure a value is left on the stack
    auto emit_body = [&](const MatchArm& arm) {
        if (arm.body_block)
        {
            arm.body_block->accept(*this);
            // Block doesn't push a value — push nil as the match result
            emit(OpCode::Constant, nil_idx, line);
        }
        else if (arm.body_expr)
        {
            arm.body_expr->accept(*this);
            // Expression result is already on the stack
        }
        else
        {
            // Empty arm (shouldn't happen) — push nil
            emit(OpCode::Constant, nil_idx, line);
        }
    };
    
    for (const auto& arm : expr.arms)
    {
        if (arm.is_default())
        {
            // Default arm: pop match value, run body (with result), jump to end.
            if (arm.guard)
            {
                // Guard on default: if guard false -> fall through to "no match" case
                arm.guard->accept(*this);
                u32 guard_fail = emit_jump(OpCode::JumpIfFalse, line);
                emit(OpCode::Pop, line); // pop match value
                emit_body(arm);
                end_jumps.push_back(emit_jump(OpCode::Jump, line));
                patch_jump(guard_fail);
            }
            else
            {
                emit(OpCode::Pop, line); // pop match value
                emit_body(arm);
                end_jumps.push_back(emit_jump(OpCode::Jump, line));
            }
        }
        else
        {
            // Non-default arm with or-patterns [p1, p2, ..., pN].
            // Algorithm:
            //   For each pattern pi (except last):
            //     Dup, pi, Equal
            //     JumpIfFalse -> try_next_pi  (miss this pattern)
            //     Jump -> body_label           (hit! go to body)
            //     try_next_pi:
            //   For last pattern pN:
            //     Dup, pN, Equal
            //     JumpIfFalse -> next_arm_label (miss all patterns)
            //   body_label:
            //     [optional guard: if guard fails -> next_arm_label]
            //     Pop (match value), run body, push result, Jump end
            //   next_arm_label:

            std::vector<u32> to_body_jumps; // all "hit" jumps pointing to body
            u32 to_next_arm = 0;            // "miss all" jump pointing to next arm
            
            size_t n = arm.patterns.size();
            for (size_t pi = 0; pi < n; ++pi)
            {
                emit(OpCode::Dup, line);
                arm.patterns[pi]->accept(*this);
                emit(OpCode::Equal, line);
                
                if (pi + 1 < n)
                {
                    // Not the last pattern: JumpIfFalse -> try next; Jump -> body
                    u32 miss = emit_jump(OpCode::JumpIfFalse, line);
                    to_body_jumps.push_back(emit_jump(OpCode::Jump, line));
                    patch_jump(miss); // miss this pattern: continue to next
                }
                else
                {
                    // Last pattern: JumpIfFalse -> next arm (miss all)
                    to_next_arm = emit_jump(OpCode::JumpIfFalse, line);
                }
            }
            
            // Patch all "hit" jumps to here (body start)
            for (u32 j : to_body_jumps)
                patch_jump(j);
            
            // Optional guard
            if (arm.guard)
            {
                arm.guard->accept(*this);
                // If guard fails, jump to next arm
                u32 guard_fail = emit_jump(OpCode::JumpIfFalse, line);
                
                // Guard passed: pop match value, run body (with result), jump to end
                emit(OpCode::Pop, line);
                emit_body(arm);
                end_jumps.push_back(emit_jump(OpCode::Jump, line));
                
                // Guard failed or pattern missed: land here -> next arm
                patch_jump(guard_fail);
                patch_jump(to_next_arm);
            }
            else
            {
                // No guard: pop match value, run body (with result), jump to end
                emit(OpCode::Pop, line);
                emit_body(arm);
                end_jumps.push_back(emit_jump(OpCode::Jump, line));
                
                // All patterns missed: land here -> next arm
                patch_jump(to_next_arm);
            }
        }
    }
    
    // No arm matched (or no default arm): pop match value, push nil as result.
    emit(OpCode::Pop, line);
    emit(OpCode::Constant, nil_idx, line);
    
    // All arm bodies jump here (past the fallback nil push)
    for (u32 jump : end_jumps)
        patch_jump(jump);
    
    return std::any();
}
std::any IRGenerator::visit(const BreakStmt& stmt)
{
    if (m_loop_stack.empty())
    {
        // Validated by parser, but guard anyway
        return std::any();
    }
    // Pop any locals introduced since the loop started
    auto& ctx = m_loop_stack.back();
    int locals_to_pop = 0;
    for (int i = static_cast<int>(current()->locals.size()) - 1; i >= 0; --i)
    {
        if (current()->locals[i].depth > ctx.scope_depth)
            ++locals_to_pop;
        else
            break;
    }
    for (int i = 0; i < locals_to_pop; ++i)
        emit(OpCode::Pop, stmt.keyword.span.start.line);

    // Emit a jump placeholder; record it so the loop can patch it
    u32 jump_idx = emit_jump(OpCode::Jump, stmt.keyword.span.start.line);
    m_loop_stack.back().break_jumps.push_back(jump_idx);
    return std::any();
}

std::any IRGenerator::visit(const ContinueStmt& stmt)
{
    if (m_loop_stack.empty())
    {
        return std::any();
    }
    auto& ctx = m_loop_stack.back();
    // Pop any locals introduced since loop entry
    int locals_to_pop = 0;
    for (int i = static_cast<int>(current()->locals.size()) - 1; i >= 0; --i)
    {
        if (current()->locals[i].depth > ctx.scope_depth)
            ++locals_to_pop;
        else
            break;
    }
    for (int i = 0; i < locals_to_pop; ++i)
        emit(OpCode::Pop, stmt.keyword.span.start.line);

    // Loop back to loop_start (increment section for for-loops, condition for while)
    emit_loop(ctx.loop_start, stmt.keyword.span.start.line);
    return std::any();
}

} // namespace blades
