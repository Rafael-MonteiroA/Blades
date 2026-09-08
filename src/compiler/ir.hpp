#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// ir.hpp — Intermediate Representation (IR) structures for Blades
// ─────────────────────────────────────────────────────────────────────────────

#include <vector>
#include <string>
#include <cstdint>

#include "compiler/token.hpp"
#include "common/types.hpp"
#include "backend/value.hpp"

namespace blades
{

enum class OpCode : u8
{
    // Constants
    Constant,

    // Math
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Negate,
    BitAnd,
    BitOr,
    BitXor,
    BitNot,
    ShiftLeft,
    ShiftRight,

    // Logic
    Not,
    Equal,
    NotEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,

    // Variables (using string names for simplicity in early IR, or indices)
    // We will use string indices in the constant pool for globals
    DefineGlobal,
    GetGlobal,
    SetGlobal,

    GetLocal,
    SetLocal,
    Pop,
    Dup,

    // Control flow
    Jump,
    JumpIfFalse,
    Loop,

    Yield, // Suspend current fiber

    // Functions & Closures
    Call,
    Closure,
    GetUpvalue,
    SetUpvalue,
    CloseUpvalue,
    
    // Arrays & Dicts
    BuildList,
    BuildDict,
    GetSubscript,
    SetSubscript,
    GetProperty,
    SetProperty,

    // Classes
    Class,
    Method,
    Inherit,
    GetSuper,

    // End of instruction
    Return,

    // Loop control
    Break,
    Continue
};

struct Instruction
{
    OpCode op;
    u32 operand;      // e.g., constant index, local slot, or jump offset
    u32 source_line;  // for debug/errors
};

class IRChunk
{
public:
    std::vector<Instruction> code;
    
    // We store literal values in a constant pool
    std::vector<Value> constants;

    void write(OpCode op, u32 operand, u32 line);
    u32 add_constant(Value value);
};

std::string disassemble_chunk(const IRChunk& chunk, const std::string& name);
std::string disassemble_instruction(const IRChunk& chunk, u32 offset);

struct CapturedUpvalue {
    u32 index;
    bool is_local;
};

struct ObjFunction
{
    u32 arity = 0;
    std::string name;
    std::string source_name;
    IRChunk chunk;
    std::vector<CapturedUpvalue> captured_upvalues;
};

} // namespace blades
