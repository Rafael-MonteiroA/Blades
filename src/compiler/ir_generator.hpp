#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// ir_generator.hpp — Translates AST to Intermediate Representation (IR)
// ─────────────────────────────────────────────────────────────────────────────

#include <vector>
#include <string>
#include <string_view>
#include <memory>
#include <any>

#include "compiler/ast.hpp"
#include "compiler/ir.hpp"

namespace blades
{

struct Local
{
    std::string name;
    int depth;
    bool is_captured = false;
};

struct CompilerState
{
    std::shared_ptr<ObjFunction> function;
    std::vector<Local> locals;
    std::vector<CapturedUpvalue> upvalues;
    int scope_depth = 0;
};

class IRGenerator : public AstVisitor
{
public:
    std::shared_ptr<ObjFunction> generate(const std::vector<std::unique_ptr<Stmt>>& statements,
                                          std::string_view source_name = {});

    // Expressions
    std::any visit(const LiteralExpr& expr) override;
    std::any visit(const BinaryExpr& expr) override;
    std::any visit(const UnaryExpr& expr) override;
    std::any visit(const GroupingExpr& expr) override;
    std::any visit(const VariableExpr& expr) override;
    std::any visit(const AssignExpr& expr) override;
    std::any visit(const CallExpr& expr) override;
    std::any visit(const ArrayExpr& expr) override;
    std::any visit(const SubscriptExpr& expr) override;
    std::any visit(const SubscriptAssignExpr& expr) override;
    std::any visit(const DictExpr& expr) override;
    std::any visit(const PropertyExpr& expr) override;
    std::any visit(const PropertyAssignExpr& expr) override;
    std::any visit(const ThisExpr& expr) override;
    std::any visit(const FnExpr& expr) override;
    std::any visit(const YieldExpr& expr) override;
    std::any visit(const MatchExpr& expr) override;
    std::any visit(const SuperExpr& expr) override;

    // Statements
    std::any visit(const ExprStmt& stmt) override;
    std::any visit(const LetStmt& stmt) override;
    std::any visit(const BlockStmt& stmt) override;
    std::any visit(const IfStmt& stmt) override;
    std::any visit(const WhileStmt& stmt) override;
    std::any visit(const ForStmt& stmt) override;
    std::any visit(const ReturnStmt& stmt) override;
    std::any visit(const FunctionDecl& decl) override;
    std::any visit(const ClassDecl& decl) override;
    std::any visit(const ImportStmt& stmt) override;
    std::any visit(const BreakStmt& stmt) override;
    std::any visit(const ContinueStmt& stmt) override;

private:
    std::vector<std::unique_ptr<CompilerState>> m_compiler_stack;

    // Loop tracking for break/continue
    struct LoopContext {
        u32 loop_start;              // IP of loop start (for continue)
        std::vector<u32> break_jumps; // Indices of break jumps to patch
        int scope_depth;             // Scope depth at loop entry
    };
    std::vector<LoopContext> m_loop_stack;

    CompilerState* current() { return m_compiler_stack.back().get(); }
    IRChunk* current_chunk() { return &current()->function->chunk; }

    void emit(OpCode op, u32 line)
    {
        current_chunk()->write(op, 0, line);
    }
    
    void emit(OpCode op, u32 operand, u32 line)
    {
        current_chunk()->write(op, operand, line);
    }
    
    u32 emit_jump(OpCode op, u32 line)
    {
        emit(op, 0xffff, line); // Placeholder
        return static_cast<u32>(current_chunk()->code.size() - 1);
    }

    void emit_constant(Value value, u32 line)
    {
        u32 idx = make_constant(std::move(value));
        emit(OpCode::Constant, idx, line);
    }
    
    void patch_jump(u32 offset)
    {
        // Jump is relative to the instruction AFTER the jump instruction
        u32 jump = static_cast<u32>(current_chunk()->code.size()) - 1 - offset;
        current_chunk()->code[offset].operand = jump;
    }
    
    void emit_loop(u32 loop_start, u32 line)
    {
        // loop_start is absolute index, we calculate offset backwards from IP AFTER instruction
        u32 jump = static_cast<u32>(current_chunk()->code.size()) + 1 - loop_start;
        emit(OpCode::Loop, jump, line);
    }

    u32 make_constant(Value value)
    {
        return current_chunk()->add_constant(std::move(value));
    }
    
    int resolve_local(const std::string& name)
    {
        auto* comp = current();
        for (int i = static_cast<int>(comp->locals.size()) - 1; i >= 0; --i)
        {
            if (comp->locals[i].name == name)
            {
                return i;
            }
        }
        return -1;
    }
    
    int resolve_local(CompilerState* comp, const std::string& name)
    {
        for (int i = static_cast<int>(comp->locals.size()) - 1; i >= 0; --i)
        {
            if (comp->locals[i].name == name)
            {
                return i;
            }
        }
        return -1;
    }

    int add_upvalue(CompilerState* comp, u32 index, bool is_local)
    {
        for (size_t i = 0; i < comp->upvalues.size(); i++)
        {
            if (comp->upvalues[i].index == index && comp->upvalues[i].is_local == is_local)
                return static_cast<int>(i);
        }
        comp->upvalues.push_back({index, is_local});
        return static_cast<int>(comp->upvalues.size() - 1);
    }

    int resolve_upvalue(int compiler_index, const std::string& name)
    {
        if (compiler_index <= 0) return -1;

        CompilerState* enclosing = m_compiler_stack[compiler_index - 1].get();
        
        int local = resolve_local(enclosing, name);
        if (local != -1)
        {
            enclosing->locals[local].is_captured = true;
            return add_upvalue(m_compiler_stack[compiler_index].get(), static_cast<u32>(local), true);
        }

        int upvalue = resolve_upvalue(compiler_index - 1, name);
        if (upvalue != -1)
        {
            return add_upvalue(m_compiler_stack[compiler_index].get(), static_cast<u32>(upvalue), false);
        }

        return -1;
    }
};

} // namespace blades
