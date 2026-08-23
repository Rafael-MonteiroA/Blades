#pragma once

#include "framework/test.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"

namespace blades::test
{

inline std::string parse_and_print(std::string_view source)
{
    Lexer lexer(source, "test.bl");
    Parser parser(lexer);
    auto stmts = parser.parse();
    
    if (stmts.empty()) return "";
    
    AstPrinter printer;
    std::string result;
    for (size_t i = 0; i < stmts.size(); ++i)
    {
        if (i > 0) result += " ";
        result += printer.print(*stmts[i]);
    }
    return result;
}

TEST_CASE("Parser - math expressions")
{
    ASSERT_EQ(parse_and_print("1 + 2 * 3;"), "(expr-stmt (+ 1 (* 2 3)))");
    ASSERT_EQ(parse_and_print("(1 + 2) * 3;"), "(expr-stmt (* (group (+ 1 2)) 3))");
    ASSERT_EQ(parse_and_print("-1 + 2;"), "(expr-stmt (+ (- 1) 2))");
    ASSERT_EQ(parse_and_print("1 - 2 - 3;"), "(expr-stmt (- (- 1 2) 3))");
}

TEST_CASE("Parser - logic and comparisons")
{
    ASSERT_EQ(parse_and_print("true and false;"), "(expr-stmt (and true false))");
    ASSERT_EQ(parse_and_print("1 == 2 or 3 != 4;"), "(expr-stmt (or (== 1 2) (!= 3 4)))");
    ASSERT_EQ(parse_and_print("!true;"), "(expr-stmt (! true))");
}

TEST_CASE("Parser - variables and assignment")
{
    ASSERT_EQ(parse_and_print("let x = 10;"), "(let x 10)");
    ASSERT_EQ(parse_and_print("let y;"), "(let y)");
    ASSERT_EQ(parse_and_print("x = 20;"), "(expr-stmt (= x 20))");
}

TEST_CASE("Parser - control flow")
{
    ASSERT_EQ(parse_and_print("if (true) { let x = 1; } else { x = 2; }"), 
              "(if true (block (let x 1)) (block (expr-stmt (= x 2))))");
              
    ASSERT_EQ(parse_and_print("while (x < 10) { x = x + 1; }"), 
              "(while (< x 10) (block (expr-stmt (= x (+ x 1)))))");
}

TEST_CASE("Parser - functions")
{
    ASSERT_EQ(parse_and_print("fn add(a, b) { return a + b; }"), 
              "(fn add (a b) (block (return (+ a b))))");
              
    ASSERT_EQ(parse_and_print("add(1, 2);"), "(expr-stmt (call add 1 2))");
}

} // namespace blades::test
