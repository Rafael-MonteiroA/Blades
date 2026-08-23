#include "compiler/ir.hpp"

#include <sstream>
#include <iomanip>

namespace blades
{

void IRChunk::write(OpCode op, u32 operand, u32 line)
{
    code.push_back(Instruction{op, operand, line});
}

u32 IRChunk::add_constant(Value value)
{
    constants.push_back(std::move(value));
    return static_cast<u32>(constants.size() - 1);
}

static std::string simple_instruction(const char* name)
{
    return std::string(name);
}

static std::string constant_instruction(const char* name, const IRChunk& chunk, u32 operand)
{
    std::ostringstream oss;
    oss << std::left << std::setw(16) << name << " " 
        << std::setw(4) << operand << " '" << to_string(chunk.constants[operand]) << "'";
    return oss.str();
}

static std::string byte_instruction(const char* name, u32 operand)
{
    std::ostringstream oss;
    oss << std::left << std::setw(16) << name << " " << operand;
    return oss.str();
}

static std::string jump_instruction(const char* name, int sign, const IRChunk& chunk, u32 offset, u32 operand)
{
    (void)chunk;
    std::ostringstream oss;
    u32 jump = offset + 1 + (sign * operand); // +1 because IP moves past this instruction
    oss << std::left << std::setw(16) << name << " " << operand << " -> " << jump;
    return oss.str();
}

std::string disassemble_instruction(const IRChunk& chunk, u32 offset)
{
    const Instruction& inst = chunk.code[offset];

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << offset << std::setfill(' ') << " ";

    if (offset > 0 && inst.source_line == chunk.code[offset - 1].source_line)
    {
        oss << "   | ";
    }
    else
    {
        oss << std::setw(4) << inst.source_line << " ";
    }

    switch (inst.op)
    {
        case OpCode::Constant:     return oss.str() + constant_instruction("OP_CONSTANT", chunk, inst.operand);
        case OpCode::Add:          return oss.str() + simple_instruction("OP_ADD");
        case OpCode::Subtract:     return oss.str() + simple_instruction("OP_SUBTRACT");
        case OpCode::Multiply:     return oss.str() + simple_instruction("OP_MULTIPLY");
        case OpCode::Divide:       return oss.str() + simple_instruction("OP_DIVIDE");
        case OpCode::Negate:       return oss.str() + simple_instruction("OP_NEGATE");
        case OpCode::Not:          return oss.str() + simple_instruction("OP_NOT");
        case OpCode::Equal:        return oss.str() + simple_instruction("OP_EQUAL");
        case OpCode::NotEqual:     return oss.str() + simple_instruction("OP_NOT_EQUAL");
        case OpCode::Greater:      return oss.str() + simple_instruction("OP_GREATER");
        case OpCode::GreaterEqual: return oss.str() + simple_instruction("OP_GREATER_EQUAL");
        case OpCode::Less:         return oss.str() + simple_instruction("OP_LESS");
        case OpCode::LessEqual:    return oss.str() + simple_instruction("OP_LESS_EQUAL");
        
        case OpCode::DefineGlobal: return oss.str() + constant_instruction("OP_DEFINE_GLOBAL", chunk, inst.operand);
        case OpCode::GetGlobal:    return oss.str() + constant_instruction("OP_GET_GLOBAL", chunk, inst.operand);
        case OpCode::SetGlobal:    return oss.str() + constant_instruction("OP_SET_GLOBAL", chunk, inst.operand);
        
        case OpCode::GetLocal:     return oss.str() + byte_instruction("OP_GET_LOCAL", inst.operand);
        case OpCode::SetLocal:     return oss.str() + byte_instruction("OP_SET_LOCAL", inst.operand);
        case OpCode::Pop:          return oss.str() + simple_instruction("OP_POP");
        
        case OpCode::BuildList:    return oss.str() + byte_instruction("OP_BUILD_LIST", inst.operand);
        case OpCode::GetSubscript: return oss.str() + simple_instruction("OP_GET_SUBSCRIPT");
        case OpCode::SetSubscript: return oss.str() + simple_instruction("OP_SET_SUBSCRIPT");
        
        case OpCode::Jump:         return oss.str() + jump_instruction("OP_JUMP", 1, chunk, offset, inst.operand);
        case OpCode::JumpIfFalse:  return oss.str() + jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset, inst.operand);
        case OpCode::Loop:         return oss.str() + jump_instruction("OP_LOOP", -1, chunk, offset, inst.operand);
        
        case OpCode::Call:         return oss.str() + byte_instruction("OP_CALL", inst.operand);
        case OpCode::Return:       return oss.str() + simple_instruction("OP_RETURN");
        
        default:                   return oss.str() + "Unknown opcode";
    }
}

std::string disassemble_chunk(const IRChunk& chunk, const std::string& name)
{
    std::ostringstream oss;
    oss << "== " << name << " ==\n";
    for (u32 offset = 0; offset < chunk.code.size(); ++offset)
    {
        oss << disassemble_instruction(chunk, offset) << "\n";
    }
    return oss.str();
}

} // namespace blades
