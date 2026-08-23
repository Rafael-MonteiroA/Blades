#pragma once

#include "framework/test.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/semantic_analyzer.hpp"

namespace blades::test
{

inline void analyze(std::string_view source)
{
    Lexer lexer(source, "test.bl");
    Parser parser(lexer);
    auto stmts = parser.parse();
    
    SymbolTable globals;
    SemanticAnalyzer analyzer(globals);
    analyzer.analyze(stmts);
}

TEST_CASE("Semantic - variable declaration and assignment")
{
    // Valid
    analyze("let x = 10; x = 20;");
}

TEST_CASE("Semantic - undefined variable")
{
    ASSERT_THROWS(analyze("x = 10;"));
    ASSERT_THROWS(analyze("let y = x;"));
}

TEST_CASE("Semantic - type mismatch on assignment")
{
    ASSERT_THROWS(analyze("let x = 10; x = \"hello\";"));
}

TEST_CASE("Semantic - binary ops type checking")
{
    // Valid
    analyze("let x = 1 + 2;");
    analyze("let y = 1.0 + 2.0;");
    
    // Invalid
    ASSERT_THROWS(analyze("let x = 1 + \"hello\";"));
    ASSERT_THROWS(analyze("let y = true + false;"));
}

TEST_CASE("Semantic - logical ops type checking")
{
    // Valid
    analyze("let x = true and false;");
    
    // Invalid
    ASSERT_THROWS(analyze("let x = 1 and 2;"));
}

TEST_CASE("Semantic - conditions must be boolean")
{
    analyze("if (true) {}");
    analyze("while (false) {}");
    
    ASSERT_THROWS(analyze("if (1) {}"));
    ASSERT_THROWS(analyze("while (\"yes\") {}"));
}

TEST_CASE("Semantic - scoping")
{
    // Valid: shadowing or separate scopes
    analyze("{ let x = 1; } { let x = 2; }");
    
    // Invalid: out of scope
    ASSERT_THROWS(analyze("{ let x = 1; } x = 2;"));
}

} // namespace blades::test
