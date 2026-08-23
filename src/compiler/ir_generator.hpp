#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// ir_generator.hpp — Translates AST to Intermediate Representation (IR)
// ─────────────────────────────────────────────────────────────────────────────

#include <vector>
#include <string>
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
};

struct CompilerState
{
    std::shared_ptr<ObjFunction> function;
    std::vector<Local> locals;
    int scope_depth = 0;
};

class IRGenerator : public AstVisitor
{
public:
    std::shared_ptr<ObjFunction> generate(const std::vector<std::unique_ptr<Stmt>>& statements);

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

private:
    std::vector<std::unique_ptr<CompilerState>> m_compiler_stack;

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
};

} // namespace blades
