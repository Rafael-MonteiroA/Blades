#include "backend/vm.hpp"

#include <iostream>
#include <sstream>

namespace blades
{

void VM::push(Value value)
{
    m_stack.push_back(std::move(value));
}

Value VM::pop()
{
    Value value = std::move(m_stack.back());
    m_stack.pop_back();
    return value;
}

Value VM::peek(int distance) const
{
    return m_stack[m_stack.size() - 1 - distance];
}

void VM::runtime_error(const char* message)
{
    std::cerr << "Runtime Error: " << message << "\n";
    if (m_chunk && m_ip < m_chunk->code.size())
    {
        std::cerr << "[line " << m_chunk->code[m_ip].source_line << "] in script\n";
    }
}

void VM::define_native(const std::string& name, NativeFn function)
{
    m_globals[name] = Value(function);
}

InterpretResult VM::interpret(const IRChunk& chunk)
{
    m_chunk = &chunk;
    m_ip = 0;
    return run();
}

InterpretResult VM::run()
{
    #define READ_INSTRUCTION() (m_chunk->code[m_ip++])
    #define READ_CONSTANT(operand) (m_chunk->constants[operand])
    
    while (m_ip < m_chunk->code.size())
    {
        Instruction inst = READ_INSTRUCTION();
        
        switch (inst.op)
        {
            case OpCode::Constant:
            {
                Value constant = READ_CONSTANT(inst.operand);
                push(constant);
                break;
            }
            case OpCode::Add:
            {
                Value b = pop();
                Value a = pop();
                if (a.is_number() && b.is_number())
                {
                    if (a.is_double() || b.is_double())
                    {
                        push(Value(a.as_number() + b.as_number()));
                    }
                    else
                    {
                        push(Value(a.as_int() + b.as_int()));
                    }
                }
                else if (a.is_string() && b.is_string())
                {
                    push(Value(a.as_string() + b.as_string()));
                }
                else
                {
                    runtime_error("Operands must be two numbers or two strings.");
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OpCode::Subtract:
            {
                Value b = pop();
                Value a = pop();
                if (a.is_number() && b.is_number())
                {
                    if (a.is_double() || b.is_double())
                    {
                        push(Value(a.as_number() - b.as_number()));
                    }
                    else
                    {
                        push(Value(a.as_int() - b.as_int()));
                    }
                }
                else
                {
                    runtime_error("Operands must be numbers.");
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OpCode::Multiply:
            {
                Value b = pop();
                Value a = pop();
                if (a.is_number() && b.is_number())
                {
                    if (a.is_double() || b.is_double())
                    {
                        push(Value(a.as_number() * b.as_number()));
                    }
                    else
                    {
                        push(Value(a.as_int() * b.as_int()));
                    }
                }
                else
                {
                    runtime_error("Operands must be numbers.");
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OpCode::Divide:
            {
                Value b = pop();
                Value a = pop();
                if (a.is_number() && b.is_number())
                {
                    if (b.as_number() == 0)
                    {
                        runtime_error("Division by zero.");
                        return InterpretResult::RuntimeError;
                    }
                    
                    if (a.is_double() || b.is_double())
                    {
                        push(Value(a.as_number() / b.as_number()));
                    }
                    else
                    {
                        push(Value(a.as_int() / b.as_int()));
                    }
                }
                else
                {
                    runtime_error("Operands must be numbers.");
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OpCode::Negate:
            {
                if (!peek(0).is_number())
                {
                    runtime_error("Operand must be a number.");
                    return InterpretResult::RuntimeError;
                }
                
                Value v = pop();
                if (v.is_double()) push(Value(-v.as_double()));
                else push(Value(-v.as_int()));
                break;
            }
            case OpCode::Not:
            {
                Value v = pop();
                if (v.is_bool()) push(Value(!v.as_bool()));
                else if (v.is_nil()) push(Value(true));
                else push(Value(false)); // Truthiness: non-nil/non-false are true, so Not is false
                break;
            }
            case OpCode::Equal:
            {
                Value b = pop();
                Value a = pop();
                push(Value(a == b));
                break;
            }
            case OpCode::NotEqual:
            {
                Value b = pop();
                Value a = pop();
                push(Value(a != b));
                break;
            }
            case OpCode::Greater:
            {
                Value b = pop();
                Value a = pop();
                if (a.is_number() && b.is_number()) push(Value(a.as_number() > b.as_number()));
                else { runtime_error("Operands must be numbers."); return InterpretResult::RuntimeError; }
                break;
            }
            case OpCode::GreaterEqual:
            {
                Value b = pop();
                Value a = pop();
                if (a.is_number() && b.is_number()) push(Value(a.as_number() >= b.as_number()));
                else { runtime_error("Operands must be numbers."); return InterpretResult::RuntimeError; }
                break;
            }
            case OpCode::Less:
            {
                Value b = pop();
                Value a = pop();
                if (a.is_number() && b.is_number()) push(Value(a.as_number() < b.as_number()));
                else { runtime_error("Operands must be numbers."); return InterpretResult::RuntimeError; }
                break;
            }
            case OpCode::LessEqual:
            {
                Value b = pop();
                Value a = pop();
                if (a.is_number() && b.is_number()) push(Value(a.as_number() <= b.as_number()));
                else { runtime_error("Operands must be numbers."); return InterpretResult::RuntimeError; }
                break;
            }
            case OpCode::DefineGlobal:
            {
                std::string name = READ_CONSTANT(inst.operand).as_string();
                m_globals[name] = pop();
                break;
            }
            case OpCode::GetGlobal:
            {
                std::string name = READ_CONSTANT(inst.operand).as_string();
                auto it = m_globals.find(name);
                if (it == m_globals.end())
                {
                    runtime_error(("Undefined variable '" + name + "'.").c_str());
                    return InterpretResult::RuntimeError;
                }
                push(it->second);
                break;
            }
            case OpCode::SetGlobal:
            {
                std::string name = READ_CONSTANT(inst.operand).as_string();
                if (m_globals.find(name) == m_globals.end())
                {
                    runtime_error(("Undefined variable '" + name + "'.").c_str());
                    return InterpretResult::RuntimeError;
                }
                m_globals[name] = peek(0);
                break;
            }
            case OpCode::GetLocal:
            {
                push(m_stack[inst.operand]);
                break;
            }
            case OpCode::SetLocal:
            {
                m_stack[inst.operand] = peek(0);
                break;
            }
            case OpCode::Jump:
            {
                u32 offset = inst.operand;
                m_ip += offset;
                break;
            }
            case OpCode::JumpIfFalse:
            {
                u32 offset = inst.operand;
                Value condition = peek(0);
                
                // Pop the condition? Wait, IR block statement doesn't pop it automatically yet, 
                // but actually our IR compiler pops or leaves it. If we leave it on stack, we have stack leak.
                // Normally a jumpIfFalse leaves the value on stack for short circuit OR/AND, but for IF it pops.
                // Our IRGenerator doesn't emit POP. Let's just pop it here for simplicity in this minimal VM.
                pop();
                
                bool is_truthy = true;
                if (condition.is_nil()) is_truthy = false;
                else if (condition.is_bool()) is_truthy = condition.as_bool();
                
                if (!is_truthy)
                {
                    m_ip += offset;
                }
                break;
            }
            case OpCode::Loop:
            {
                u32 offset = inst.operand;
                m_ip -= offset;
                break;
            }
            case OpCode::Return:
            {
                // We're done (for a single top-level script chunk)
                return InterpretResult::Ok;
            }
            case OpCode::Call:
            {
                u32 arg_count = inst.operand;
                std::vector<Value> args(arg_count);
                
                // Pop arguments in reverse order (top of stack is last argument)
                for (int i = arg_count - 1; i >= 0; --i)
                {
                    args[i] = pop();
                }
                
                Value callee = pop();
                if (callee.is_native_fn())
                {
                    NativeFn native = callee.as_native_fn();
                    Value result = native(args);
                    push(result);
                }
                else
                {
                    runtime_error("Can only call functions and classes.");
                    return InterpretResult::RuntimeError;
                }
                break;
            }
        }
    }
    
    #undef READ_INSTRUCTION
    #undef READ_CONSTANT
    
    return InterpretResult::Ok;
}

} // namespace blades
