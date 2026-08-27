#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// ast.hpp — Abstract Syntax Tree (AST) definitions for Blades
// ─────────────────────────────────────────────────────────────────────────────

#include <memory>
#include <vector>
#include <any>

#include "compiler/token.hpp"
#include "compiler/value_type.hpp"

namespace blades
{

// Forward declarations
class Expr;
class LiteralExpr;
class BinaryExpr;
class UnaryExpr;
class GroupingExpr;
class VariableExpr;
class AssignExpr;
class CallExpr;
class ArrayExpr;
class SubscriptExpr;
class SubscriptAssignExpr;
class DictExpr;
class PropertyExpr;
class PropertyAssignExpr;
class ThisExpr;
class FnExpr;
class YieldExpr;
class MatchExpr;
class SuperExpr;

class Stmt;
class ExprStmt;
class LetStmt;
class BlockStmt;
class IfStmt;
class WhileStmt;
class ForStmt;
class ReturnStmt;
class FunctionDecl;
class ClassDecl;
class ImportStmt;
class BreakStmt;
class ContinueStmt;

// ─────────────────────────────────────────────────────────────────────────────
// AstVisitor
// ─────────────────────────────────────────────────────────────────────────────

class AstVisitor
{
public:
    virtual ~AstVisitor() = default;

    // Expressions
    virtual std::any visit(const LiteralExpr& expr) = 0;
    virtual std::any visit(const BinaryExpr& expr) = 0;
    virtual std::any visit(const UnaryExpr& expr) = 0;
    virtual std::any visit(const GroupingExpr& expr) = 0;
    virtual std::any visit(const VariableExpr& expr) = 0;
    virtual std::any visit(const AssignExpr& expr) = 0;
    virtual std::any visit(const CallExpr& expr) = 0;
    virtual std::any visit(const ArrayExpr& expr) = 0;
    virtual std::any visit(const SubscriptExpr& expr) = 0;
    virtual std::any visit(const SubscriptAssignExpr& expr) = 0;
    virtual std::any visit(const DictExpr& expr) = 0;
    virtual std::any visit(const PropertyExpr& expr) = 0;
    virtual std::any visit(const PropertyAssignExpr& expr) = 0;
    virtual std::any visit(const ThisExpr& expr) = 0;
    virtual std::any visit(const FnExpr& expr) = 0;
    virtual std::any visit(const YieldExpr& expr) = 0;
    virtual std::any visit(const MatchExpr& expr) = 0;
    virtual std::any visit(const SuperExpr& expr) = 0;

    // Statements
    virtual std::any visit(const ExprStmt& stmt) = 0;
    virtual std::any visit(const LetStmt& stmt) = 0;
    virtual std::any visit(const BlockStmt& stmt) = 0;
    virtual std::any visit(const IfStmt& stmt) = 0;
    virtual std::any visit(const WhileStmt& stmt) = 0;
    virtual std::any visit(const ForStmt& stmt) = 0;
    virtual std::any visit(const ReturnStmt& stmt) = 0;
    virtual std::any visit(const FunctionDecl& decl) = 0;
    virtual std::any visit(const ClassDecl& decl) = 0;
    virtual std::any visit(const ImportStmt& stmt) = 0;
    virtual std::any visit(const BreakStmt& stmt) = 0;
    virtual std::any visit(const ContinueStmt& stmt) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// AstNode
// ─────────────────────────────────────────────────────────────────────────────

class AstNode
{
public:
    virtual ~AstNode() = default;
    virtual std::any accept(AstVisitor& visitor) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Expressions
// ─────────────────────────────────────────────────────────────────────────────

class Expr : public AstNode
{
public:
    mutable ValueType resolved_type = ValueType::Unknown;
};

class LiteralExpr : public Expr
{
public:
    Token value;

    explicit LiteralExpr(Token value) : value(std::move(value)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class BinaryExpr : public Expr
{
public:
    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class UnaryExpr : public Expr
{
public:
    Token op;
    std::unique_ptr<Expr> right;

    UnaryExpr(Token op, std::unique_ptr<Expr> right)
        : op(std::move(op)), right(std::move(right)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class GroupingExpr : public Expr
{
public:
    std::unique_ptr<Expr> expression;

    explicit GroupingExpr(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class VariableExpr : public Expr
{
public:
    Token name;

    explicit VariableExpr(Token name) : name(std::move(name)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class AssignExpr : public Expr
{
public:
    Token name;
    std::unique_ptr<Expr> value;

    AssignExpr(Token name, std::unique_ptr<Expr> value)
        : name(std::move(name)), value(std::move(value)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class CallExpr : public Expr
{
public:
    std::unique_ptr<Expr> callee;
    Token paren; // For location reporting
    std::vector<std::unique_ptr<Expr>> arguments;

    CallExpr(std::unique_ptr<Expr> callee, Token paren, std::vector<std::unique_ptr<Expr>> arguments)
        : callee(std::move(callee)), paren(std::move(paren)), arguments(std::move(arguments)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class ArrayExpr : public Expr
{
public:
    std::vector<std::unique_ptr<Expr>> elements;
    
    explicit ArrayExpr(std::vector<std::unique_ptr<Expr>> elements)
        : elements(std::move(elements)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class SubscriptExpr : public Expr
{
public:
    std::unique_ptr<Expr> object;
    std::unique_ptr<Expr> index;
    
    SubscriptExpr(std::unique_ptr<Expr> object, std::unique_ptr<Expr> index)
        : object(std::move(object)), index(std::move(index)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class SubscriptAssignExpr : public Expr
{
public:
    std::unique_ptr<Expr> object;
    std::unique_ptr<Expr> index;
    std::unique_ptr<Expr> value;
    Token bracket;

    SubscriptAssignExpr(std::unique_ptr<Expr> object, std::unique_ptr<Expr> index, std::unique_ptr<Expr> value, Token bracket)
        : object(std::move(object)), index(std::move(index)), value(std::move(value)), bracket(std::move(bracket)) {}

    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class DictExpr : public Expr
{
public:
    // A dictionary is a list of key-value pairs.
    // For simplicity, keys are represented as expressions (typically LiteralExpr with String/Identifier)
    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> elements;

    DictExpr(std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> elements)
        : elements(std::move(elements)) {}

    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class PropertyExpr : public Expr
{
public:
    std::unique_ptr<Expr> object;
    Token name;

    PropertyExpr(std::unique_ptr<Expr> object, Token name)
        : object(std::move(object)), name(std::move(name)) {}

    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class PropertyAssignExpr : public Expr
{
public:
    std::unique_ptr<Expr> object;
    Token name;
    std::unique_ptr<Expr> value;

    PropertyAssignExpr(std::unique_ptr<Expr> object, Token name, std::unique_ptr<Expr> value)
        : object(std::move(object)), name(std::move(name)), value(std::move(value)) {}

    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

// ─────────────────────────────────────────────────────────────────────────────
// ThisExpr
// ─────────────────────────────────────────────────────────────────────────────

class ThisExpr : public Expr
{
public:
    Token keyword;

    explicit ThisExpr(Token keyword) : keyword(std::move(keyword)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

// ─────────────────────────────────────────────────────────────────────────────
// FnExpr
// ─────────────────────────────────────────────────────────────────────────────

class FnExpr : public Expr
{
public:
    std::vector<Token> params;
    std::unique_ptr<BlockStmt> body;

    FnExpr(std::vector<Token> params, std::unique_ptr<BlockStmt> body)
        : params(std::move(params)), body(std::move(body)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

// ─────────────────────────────────────────────────────────────────────────────
// YieldExpr
// ─────────────────────────────────────────────────────────────────────────────

class YieldExpr : public Expr
{
public:
    Token keyword;
    std::unique_ptr<Expr> value;

    YieldExpr(Token keyword, std::unique_ptr<Expr> value)
        : keyword(std::move(keyword)), value(std::move(value)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

struct MatchArm
{
    // patterns: list of patterns for this arm (or-patterns: 1 | 2 | 3)
    // An empty patterns list represents the default '_' fallback
    std::vector<std::unique_ptr<Expr>> patterns;
    // body_expr: non-null for single-expression arms
    std::unique_ptr<Expr> body_expr;
    // body_block: non-null for block arms { ... }
    std::unique_ptr<BlockStmt> body_block;
    // guard: optional `if condition` guard
    std::unique_ptr<Expr> guard;

    // Constructor for expression body
    MatchArm(std::vector<std::unique_ptr<Expr>> patterns,
             std::unique_ptr<Expr> body_expr,
             std::unique_ptr<BlockStmt> body_block,
             std::unique_ptr<Expr> guard)
        : patterns(std::move(patterns))
        , body_expr(std::move(body_expr))
        , body_block(std::move(body_block))
        , guard(std::move(guard)) {}
    
    bool is_default() const { return patterns.empty(); }
};

class MatchExpr : public Expr
{
public:
    Token keyword;
    std::unique_ptr<Expr> value;
    std::vector<MatchArm> arms;

    MatchExpr(Token keyword, std::unique_ptr<Expr> value, std::vector<MatchArm> arms)
        : keyword(std::move(keyword)), value(std::move(value)), arms(std::move(arms)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class SuperExpr : public Expr
{
public:
    Token keyword;
    Token method;

    SuperExpr(Token keyword, Token method)
        : keyword(std::move(keyword)), method(std::move(method)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

// ─────────────────────────────────────────────────────────────────────────────
// Statements
// ─────────────────────────────────────────────────────────────────────────────

class Stmt : public AstNode
{
};

class ExprStmt : public Stmt
{
public:
    std::unique_ptr<Expr> expression;

    explicit ExprStmt(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class LetStmt : public Stmt
{
public:
    Token name;
    std::unique_ptr<Expr> initializer; // Can be null

    LetStmt(Token name, std::unique_ptr<Expr> initializer)
        : name(std::move(name)), initializer(std::move(initializer)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class BlockStmt : public Stmt
{
public:
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> statements)
        : statements(std::move(statements)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class IfStmt : public Stmt
{
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> then_branch;
    std::unique_ptr<Stmt> else_branch; // Can be null

    IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> then_branch, std::unique_ptr<Stmt> else_branch)
        : condition(std::move(condition)), then_branch(std::move(then_branch)), else_branch(std::move(else_branch)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class WhileStmt : public Stmt
{
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
        : condition(std::move(condition)), body(std::move(body)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class ForStmt : public Stmt
{
public:
    std::unique_ptr<Stmt> initializer;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> increment;
    std::unique_ptr<Stmt> body;

    ForStmt(std::unique_ptr<Stmt> init, std::unique_ptr<Expr> cond, std::unique_ptr<Expr> inc, std::unique_ptr<Stmt> b)
        : initializer(std::move(init)), condition(std::move(cond)), increment(std::move(inc)), body(std::move(b)) {}

    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class ReturnStmt : public Stmt
{
public:
    Token keyword;
    std::unique_ptr<Expr> value; // Can be null

    ReturnStmt(Token keyword, std::unique_ptr<Expr> value)
        : keyword(std::move(keyword)), value(std::move(value)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class FunctionDecl : public Stmt
{
public:
    Token name;
    std::vector<Token> params;
    std::unique_ptr<BlockStmt> body;

    FunctionDecl(Token name, std::vector<Token> params, std::unique_ptr<BlockStmt> body)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class ClassDecl : public Stmt
{
public:
    Token name;
    std::unique_ptr<VariableExpr> superclass;
    std::vector<std::unique_ptr<FunctionDecl>> methods;

    ClassDecl(Token name, std::unique_ptr<VariableExpr> superclass, std::vector<std::unique_ptr<FunctionDecl>> methods)
        : name(std::move(name)), superclass(std::move(superclass)), methods(std::move(methods)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class ImportStmt : public Stmt
{
public:
    Token keyword;
    Token path;

    ImportStmt(Token keyword, Token path)
        : keyword(std::move(keyword)), path(std::move(path)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class BreakStmt : public Stmt
{
public:
    Token keyword;

    explicit BreakStmt(Token keyword) : keyword(std::move(keyword)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

class ContinueStmt : public Stmt
{
public:
    Token keyword;

    explicit ContinueStmt(Token keyword) : keyword(std::move(keyword)) {}
    std::any accept(AstVisitor& visitor) const override { return visitor.visit(*this); }
};

// ─────────────────────────────────────────────────────────────────────────────
// AstPrinter — Helper to stringify the AST (mostly for tests)
// ─────────────────────────────────────────────────────────────────────────────

class AstPrinter : public AstVisitor
{
public:
    std::string print(const AstNode& node);
    
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
};

} // namespace blades
