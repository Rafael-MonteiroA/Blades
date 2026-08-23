#include "compiler/ast.hpp"

#include <sstream>
#include <string>

namespace blades
{

std::string AstPrinter::print(const AstNode& node)
{
    return std::any_cast<std::string>(node.accept(*this));
}

std::any AstPrinter::visit(const LiteralExpr& expr)
{
    if (expr.value.type == TokenType::True) return std::string("true");
    if (expr.value.type == TokenType::False) return std::string("false");
    return std::string(expr.value.lexeme);
}

std::any AstPrinter::visit(const BinaryExpr& expr)
{
    std::ostringstream oss;
    oss << "(" << expr.op.lexeme << " "
        << std::any_cast<std::string>(expr.left->accept(*this)) << " "
        << std::any_cast<std::string>(expr.right->accept(*this)) << ")";
    return oss.str();
}

std::any AstPrinter::visit(const UnaryExpr& expr)
{
    std::ostringstream oss;
    oss << "(" << expr.op.lexeme << " "
        << std::any_cast<std::string>(expr.right->accept(*this)) << ")";
    return oss.str();
}

std::any AstPrinter::visit(const GroupingExpr& expr)
{
    std::ostringstream oss;
    oss << "(group " << std::any_cast<std::string>(expr.expression->accept(*this)) << ")";
    return oss.str();
}

std::any AstPrinter::visit(const VariableExpr& expr)
{
    return std::string(expr.name.lexeme);
}

std::any AstPrinter::visit(const AssignExpr& expr)
{
    std::ostringstream oss;
    oss << "(= " << expr.name.lexeme << " "
        << std::any_cast<std::string>(expr.value->accept(*this)) << ")";
    return oss.str();
}

std::any AstPrinter::visit(const CallExpr& expr)
{
    std::ostringstream oss;
    oss << "(call " << std::any_cast<std::string>(expr.callee->accept(*this));
    for (const auto& arg : expr.arguments)
    {
        oss << " " << std::any_cast<std::string>(arg->accept(*this));
    }
    oss << ")";
    return oss.str();
}

std::any AstPrinter::visit(const ArrayExpr& expr)
{
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < expr.elements.size(); ++i)
    {
        if (i > 0) oss << ", ";
        oss << std::any_cast<std::string>(expr.elements[i]->accept(*this));
    }
    oss << "]";
    return oss.str();
}

std::any AstPrinter::visit(const SubscriptExpr& expr)
{
    std::ostringstream oss;
    oss << "(subscript " << std::any_cast<std::string>(expr.object->accept(*this)) 
        << " " << std::any_cast<std::string>(expr.index->accept(*this)) << ")";
    return oss.str();
}

std::any AstPrinter::visit(const SubscriptAssignExpr& expr)
{
    std::ostringstream oss;
    oss << "(= (subscript " << std::any_cast<std::string>(expr.object->accept(*this)) 
        << " " << std::any_cast<std::string>(expr.index->accept(*this)) << ") "
        << std::any_cast<std::string>(expr.value->accept(*this)) << ")";
    return oss.str();
}

std::any AstPrinter::visit(const ExprStmt& stmt)
{
    std::ostringstream oss;
    oss << "(expr-stmt " << std::any_cast<std::string>(stmt.expression->accept(*this)) << ")";
    return oss.str();
}

std::any AstPrinter::visit(const LetStmt& stmt)
{
    std::ostringstream oss;
    oss << "(let " << stmt.name.lexeme;
    if (stmt.initializer)
    {
        oss << " " << std::any_cast<std::string>(stmt.initializer->accept(*this));
    }
    oss << ")";
    return oss.str();
}

std::any AstPrinter::visit(const BlockStmt& stmt)
{
    std::ostringstream oss;
    oss << "(block";
    for (const auto& s : stmt.statements)
    {
        oss << " " << std::any_cast<std::string>(s->accept(*this));
    }
    oss << ")";
    return oss.str();
}

std::any AstPrinter::visit(const IfStmt& stmt)
{
    std::ostringstream oss;
    oss << "(if " << std::any_cast<std::string>(stmt.condition->accept(*this))
        << " " << std::any_cast<std::string>(stmt.then_branch->accept(*this));
    
    if (stmt.else_branch)
    {
        oss << " " << std::any_cast<std::string>(stmt.else_branch->accept(*this));
    }
    oss << ")";
    return oss.str();
}

std::any AstPrinter::visit(const WhileStmt& stmt)
{
    std::string condition = std::any_cast<std::string>(stmt.condition->accept(*this));
    std::string body = std::any_cast<std::string>(stmt.body->accept(*this));
    return "(while " + condition + " " + body + ")";
}

std::any AstPrinter::visit(const ForStmt& stmt)
{
    std::string init = stmt.initializer ? std::any_cast<std::string>(stmt.initializer->accept(*this)) : "(no init)";
    std::string cond = stmt.condition ? std::any_cast<std::string>(stmt.condition->accept(*this)) : "(no cond)";
    std::string inc = stmt.increment ? std::any_cast<std::string>(stmt.increment->accept(*this)) : "(no inc)";
    std::string body = std::any_cast<std::string>(stmt.body->accept(*this));
    return "(for " + init + " ; " + cond + " ; " + inc + " " + body + ")";
}

std::any AstPrinter::visit(const ReturnStmt& stmt)
{
    std::ostringstream oss;
    oss << "(return";
    if (stmt.value)
    {
        oss << " " << std::any_cast<std::string>(stmt.value->accept(*this));
    }
    oss << ")";
    return oss.str();
}

std::any AstPrinter::visit(const FunctionDecl& decl)
{
    std::ostringstream oss;
    oss << "(fn " << decl.name.lexeme << " (";
    for (size_t i = 0; i < decl.params.size(); ++i)
    {
        if (i > 0) oss << " ";
        oss << decl.params[i].lexeme;
    }
    oss << ") " << std::any_cast<std::string>(decl.body->accept(*this)) << ")";
    return oss.str();
}

} // namespace blades
