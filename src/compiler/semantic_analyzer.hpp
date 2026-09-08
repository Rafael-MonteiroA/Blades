#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// semantic_analyzer.hpp — Type checking and scope resolution
// ─────────────────────────────────────────────────────────────────────────────

#include <vector>
#include <stdexcept>
#include <memory>
#include <optional>
#include <unordered_map>
#include <string_view>

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
    struct FunctionSignature
    {
        std::vector<ValueType> parameters;
        ValueType return_type = ValueType::Unknown;
    };

    using FunctionSignatureTable = std::unordered_map<std::string, FunctionSignature>;

    explicit SemanticAnalyzer(SymbolTable& globals,
                              FunctionSignatureTable* known_functions = nullptr)
        : m_symbols(globals), m_known_functions(known_functions) {}
    
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
    SymbolTable& m_symbols;

    FunctionSignatureTable m_functions;
    FunctionSignatureTable* m_known_functions = nullptr;
    std::vector<ValueType> m_return_types;
    
    // Helpers
    void error(const Token& token, const std::string& message);
    ValueType evaluate(const Expr& expr);
    void execute(const Stmt& stmt);
    ValueType type_from_annotation(const std::optional<Token>& annotation);
    bool compatible(ValueType expected, ValueType actual) const;
    const FunctionSignature* find_function(std::string_view name) const;
};

} // namespace blades
