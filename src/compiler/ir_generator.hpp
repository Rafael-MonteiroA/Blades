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

class IRGenerator : public AstVisitor
{
public:
    IRChunk generate(const std::vector<std::unique_ptr<Stmt>>& statements);

    // Expressions
    std::any visit(const LiteralExpr& expr) override;
    std::any visit(const BinaryExpr& expr) override;
    std::any visit(const UnaryExpr& expr) override;
    std::any visit(const GroupingExpr& expr) override;
    std::any visit(const VariableExpr& expr) override;
    std::any visit(const AssignExpr& expr) override;
    std::any visit(const CallExpr& expr) override;

    // Statements
    std::any visit(const ExprStmt& stmt) override;
    std::any visit(const LetStmt& stmt) override;
    std::any visit(const BlockStmt& stmt) override;
    std::any visit(const IfStmt& stmt) override;
    std::any visit(const WhileStmt& stmt) override;
    std::any visit(const ReturnStmt& stmt) override;
    std::any visit(const FunctionDecl& decl) override;

private:
    IRChunk m_chunk;
    
    std::vector<Local> m_locals;
    int m_scope_depth = 0;

    void emit(OpCode op, u32 line)
    {
        m_chunk.write(op, 0, line);
    }
    
    void emit(OpCode op, u32 operand, u32 line)
    {
        m_chunk.write(op, operand, line);
    }
    
    u32 emit_jump(OpCode op, u32 line)
    {
        emit(op, 0xffff, line); // Placeholder
        return static_cast<u32>(m_chunk.code.size() - 1);
    }
    
    void patch_jump(u32 offset)
    {
        // Jump is relative to the instruction AFTER the jump instruction
        u32 jump = static_cast<u32>(m_chunk.code.size()) - 1 - offset;
        m_chunk.code[offset].operand = jump;
    }
    
    void emit_loop(u32 loop_start, u32 line)
    {
        // loop_start is absolute index, we calculate offset backwards from IP AFTER instruction
        u32 jump = static_cast<u32>(m_chunk.code.size()) + 1 - loop_start;
        emit(OpCode::Loop, jump, line);
    }

    u32 make_constant(Value value)
    {
        return m_chunk.add_constant(std::move(value));
    }
    
    int resolve_local(const std::string& name)
    {
        for (int i = static_cast<int>(m_locals.size()) - 1; i >= 0; --i)
        {
            if (m_locals[i].name == name)
            {
                return i;
            }
        }
        return -1;
    }
};

} // namespace blades
