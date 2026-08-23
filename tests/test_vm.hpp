#pragma once

#include "framework/test.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/ir_generator.hpp"
#include "backend/vm.hpp"

namespace blades::test
{

inline InterpretResult run_vm(std::string_view source, VM& vm)
{
    Lexer lexer(source, "test.bl");
    Parser parser(lexer);
    auto stmts = parser.parse();
    
    IRGenerator generator;
    auto chunk = generator.generate(stmts);
    
    return vm.interpret(chunk);
}

TEST_CASE("VM - Evaluate expressions")
{
    VM vm;
    
    // We expect the result of the expression to be stored in global 'result' for testing
    ASSERT_EQ(run_vm("let result = 1 + 2 * 3;", vm), InterpretResult::Ok);
    ASSERT_EQ(vm.get_globals().at("result").as_int(), 7);
    
    ASSERT_EQ(run_vm("let result = (1 + 2) * 3;", vm), InterpretResult::Ok);
    ASSERT_EQ(vm.get_globals().at("result").as_int(), 9);
    
    ASSERT_EQ(run_vm("let result = 10.5 - 0.5;", vm), InterpretResult::Ok);
    ASSERT_EQ(vm.get_globals().at("result").as_double(), 10.0);
}

TEST_CASE("VM - Logical and Comparisons")
{
    VM vm;
    
    ASSERT_EQ(run_vm("let result = 5 > 3;", vm), InterpretResult::Ok);
    ASSERT_EQ(vm.get_globals().at("result").as_bool(), true);
    
    ASSERT_EQ(run_vm("let result = 5 == 5;", vm), InterpretResult::Ok);
    ASSERT_EQ(vm.get_globals().at("result").as_bool(), true);
    
    ASSERT_EQ(run_vm("let result = !true;", vm), InterpretResult::Ok);
    ASSERT_EQ(vm.get_globals().at("result").as_bool(), false);
}

TEST_CASE("VM - Strings")
{
    VM vm;
    
    ASSERT_EQ(run_vm("let result = \"hello\" + \" world\";", vm), InterpretResult::Ok);
    ASSERT_EQ(vm.get_globals().at("result").as_string(), "hello world");
}

TEST_CASE("VM - Control Flow (If)")
{
    VM vm;
    
    ASSERT_EQ(run_vm("let result = 0; if (true) { result = 1; } else { result = 2; }", vm), InterpretResult::Ok);
    ASSERT_EQ(vm.get_globals().at("result").as_int(), 1);
    
    ASSERT_EQ(run_vm("let result = 0; if (false) { result = 1; } else { result = 2; }", vm), InterpretResult::Ok);
    ASSERT_EQ(vm.get_globals().at("result").as_int(), 2);
}

TEST_CASE("VM - Control Flow (While)")
{
    VM vm;
    
    const char* src = 
        "let i = 0;"
        "let sum = 0;"
        "while (i < 5) {"
        "  sum = sum + i;"
        "  i = i + 1;"
        "}";
        
    ASSERT_EQ(run_vm(src, vm), InterpretResult::Ok);
    ASSERT_EQ(vm.get_globals().at("sum").as_int(), 10); // 0 + 1 + 2 + 3 + 4 = 10
    ASSERT_EQ(vm.get_globals().at("i").as_int(), 5);
}

} // namespace blades::test
