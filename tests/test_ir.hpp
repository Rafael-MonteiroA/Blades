#pragma once

#include "framework/test.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/ir_generator.hpp"

namespace blades::test
{

inline std::string generate_ir(std::string_view source)
{
    Lexer lexer(source, "test.bl");
    Parser parser(lexer);
    auto stmts = parser.parse();
    
    IRGenerator generator;
    auto chunk = generator.generate(stmts);
    
    return disassemble_chunk(chunk, "Test");
}

TEST_CASE("IR - expressions")
{
    std::string ir = generate_ir("1 + 2 * 3;");
    // Verify it contains LOAD_CONST and ADD
    ASSERT_EQ(ir.find("OP_CONSTANT") != std::string::npos, true);
    ASSERT_EQ(ir.find("OP_ADD") != std::string::npos, true);
    ASSERT_EQ(ir.find("OP_MULTIPLY") != std::string::npos, true);
}

TEST_CASE("IR - variables")
{
    std::string ir = generate_ir("let x = 10; x = 20;");
    ASSERT_EQ(ir.find("OP_DEFINE_GLOBAL") != std::string::npos, true);
    ASSERT_EQ(ir.find("OP_SET_GLOBAL") != std::string::npos, true);
}

TEST_CASE("IR - local variables")
{
    std::string ir = generate_ir("{ let x = 10; let y = x + 1; }");
    // Inside block, should use OP_GET_LOCAL and OP_SET_LOCAL, but let generates nothing for definition if it's local (just leaves it on stack in real bytecode, here we just resolve it).
    // Let's just check if it parses and generates without crashing for now.
    ASSERT_EQ(ir.find("OP_GET_LOCAL") != std::string::npos, true);
}

TEST_CASE("IR - control flow")
{
    std::string ir = generate_ir("if (true) { let x = 1; } else { let y = 2; }");
    ASSERT_EQ(ir.find("OP_JUMP_IF_FALSE") != std::string::npos, true);
    ASSERT_EQ(ir.find("OP_JUMP") != std::string::npos, true);
    
    std::string ir2 = generate_ir("while (false) { let x = 1; }");
    ASSERT_EQ(ir2.find("OP_LOOP") != std::string::npos, true);
}

} // namespace blades::test
