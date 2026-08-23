#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// semantic_analyzer.hpp — Type checking and scope resolution
// ─────────────────────────────────────────────────────────────────────────────

#include <vector>
#include <stdexcept>
#include <memory>

#include "compiler/ast.hpp"
#include "compiler/symbol_table.hpp"

namespace blades
{

class SemanticError : public std::runtime_error
{
public:
    explicit SemanticError(const std::string& message) : std::runtime_error(message) {}
};

class SemanticAnalyzer : public AstVisitor
{
public:
    SemanticAnalyzer(SymbolTable& globals) : m_symbols(globals) {}
    
    void analyze(const std::vector<std::unique_ptr<Stmt>>& statements);

    // Expressions (Return std::any holding a ValueType)
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

    // Statements
    std::any visit(const ExprStmt& stmt) override;
    std::any visit(const LetStmt& stmt) override;
    std::any visit(const BlockStmt& stmt) override;
    std::any visit(const IfStmt& stmt) override;
    std::any visit(const WhileStmt& stmt) override;
    std::any visit(const ForStmt& stmt) override;
    std::any visit(const ReturnStmt& stmt) override;
    std::any visit(const FunctionDecl& decl) override;

private:
    SymbolTable& m_symbols;
    
    // Helpers
    void error(const Token& token, const std::string& message);
    ValueType evaluate(const Expr& expr);
    void execute(const Stmt& stmt);
};

} // namespace blades
