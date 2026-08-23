#pragma once

#include "framework/test.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/ir_generator.hpp"
#include "backend/vm.hpp"
#include "runtime/stdlib.hpp"

namespace blades::test
{

inline InterpretResult run_with_stdlib(std::string_view source, VM& vm)
{
    Lexer lexer(source, "test_runtime.bl");
    Parser parser(lexer);
    auto stmts = parser.parse();
    
    SymbolTable globals;
    // Register stdlib in semantic analyzer so it passes type check
    globals.declare("print", ValueType::Any);
    globals.declare("clock", ValueType::Any);
    globals.declare("type_of", ValueType::Any);
    
    SemanticAnalyzer semantic(globals);
    semantic.analyze(stmts);
    
    IRGenerator generator;
    auto function = generator.generate(stmts);
    
    register_stdlib(vm);
    
    return vm.interpret(function);
}

TEST_CASE("Runtime - type_of")
{
    VM vm;
    
    ASSERT_EQ(run_with_stdlib("let a = type_of(10);", vm), InterpretResult::Ok);
    ASSERT_EQ(vm.get_globals().at("a").as_string(), "int");
    
    ASSERT_EQ(run_with_stdlib("let b = type_of(10.5);", vm), InterpretResult::Ok);
    ASSERT_EQ(vm.get_globals().at("b").as_string(), "double");
    
    ASSERT_EQ(run_with_stdlib("let c = type_of(true);", vm), InterpretResult::Ok);
    ASSERT_EQ(vm.get_globals().at("c").as_string(), "bool");
    
    ASSERT_EQ(run_with_stdlib("let d = type_of(\"hello\");", vm), InterpretResult::Ok);
    ASSERT_EQ(vm.get_globals().at("d").as_string(), "string");
}

TEST_CASE("Runtime - print")
{
    VM vm;
    // We just verify it doesn't crash. We can't easily capture stdout without redirecting rdbuf.
    ASSERT_EQ(run_with_stdlib("print(\"Hello\", \"world\", 123);", vm), InterpretResult::Ok);
}

} // namespace blades::test
