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
    for (int i = static_cast<int>(m_frames.size()) - 1; i >= 0; i--)
    {
        CallFrame& frame = m_frames[i];
        u32 instruction = frame.ip > 0 ? frame.ip - 1 : 0;
        if (instruction < frame.function->chunk.code.size())
        {
            std::cerr << "[line " << frame.function->chunk.code[instruction].source_line << "] in ";
            if (frame.function->name.empty()) std::cerr << "script\n";
            else std::cerr << frame.function->name << "()\n";
        }
    }
}

void VM::define_native(const std::string& name, NativeFn function)
{
    m_globals[name] = Value(function);
}

InterpretResult VM::interpret(std::shared_ptr<ObjFunction> function)
{
    m_frames.clear();
    // The top-level script is a function. We push it to the stack first so it can be popped on return.
    push(Value(function));
    m_frames.push_back(CallFrame{function, 0, 0}); // slots_offset is 0, callee at 0
    return run();
}

InterpretResult VM::run()
{
    #define READ_INSTRUCTION() (m_frames.back().function->chunk.code[m_frames.back().ip++])
    #define READ_CONSTANT(operand) (m_frames.back().function->chunk.constants[operand])
    
    while (m_frames.back().ip < m_frames.back().function->chunk.code.size())
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
                    if (a.is_double() || b.is_double()) push(Value(a.as_number() - b.as_number()));
                    else push(Value(a.as_int() - b.as_int()));
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
                    if (a.is_double() || b.is_double()) push(Value(a.as_number() * b.as_number()));
                    else push(Value(a.as_int() * b.as_int()));
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
                    if (a.is_double() || b.is_double()) push(Value(a.as_number() / b.as_number()));
                    else push(Value(a.as_int() / b.as_int()));
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
                else push(Value(false));
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
                push(m_stack[m_frames.back().slots_offset + inst.operand]);
                break;
            }
            case OpCode::SetLocal:
            {
                u32 slot = m_frames.back().slots_offset + inst.operand;
                m_stack[slot] = peek(0);
                break;
            }
            case OpCode::BuildList:
            {
                u32 count = inst.operand;
                auto arr = std::make_shared<ObjArray>();
                arr->elements.reserve(count);
                for (u32 i = 0; i < count; ++i)
                {
                    arr->elements.push_back(m_stack[m_stack.size() - count + i]);
                }
                for (u32 i = 0; i < count; ++i)
                {
                    m_stack.pop_back();
                }
                push(Value(arr));
                break;
            }
            case OpCode::GetSubscript:
            {
                Value index = pop();
                Value object = pop();
                
                if (!object.is_array())
                {
                    std::cerr << "Runtime Error: Object is not subscriptable.\n";
                    return InterpretResult::RuntimeError;
                }
                if (!index.is_int())
                {
                    std::cerr << "Runtime Error: Array index must be an integer.\n";
                    return InterpretResult::RuntimeError;
                }
                
                int idx = index.as_int();
                auto arr = object.as_array();
                if (idx < 0 || idx >= static_cast<int>(arr->elements.size()))
                {
                    std::cerr << "Runtime Error: Index out of bounds.\n";
                    return InterpretResult::RuntimeError;
                }
                
                push(arr->elements[idx]);
                break;
            }
            case OpCode::SetSubscript:
            {
                Value value = pop();
                Value index = pop();
                Value object = pop();
                
                if (!object.is_array())
                {
                    std::cerr << "Runtime Error: Object is not subscriptable.\n";
                    return InterpretResult::RuntimeError;
                }
                if (!index.is_int())
                {
                    std::cerr << "Runtime Error: Array index must be an integer.\n";
                    return InterpretResult::RuntimeError;
                }
                
                int idx = index.as_int();
                auto arr = object.as_array();
                if (idx < 0 || idx >= static_cast<int>(arr->elements.size()))
                {
                    std::cerr << "Runtime Error: Index out of bounds.\n";
                    return InterpretResult::RuntimeError;
                }
                
                arr->elements[idx] = value;
                push(value);
                break;
            }
            case OpCode::Pop:
            {
                pop();
                break;
            }
            case OpCode::Jump:
            {
                m_frames.back().ip += inst.operand;
                break;
            }
            case OpCode::JumpIfFalse:
            {
                Value condition = peek(0);
                pop(); // Consome a condicao do stack (simples para este compilador)
                bool is_truthy = true;
                if (condition.is_nil()) is_truthy = false;
                else if (condition.is_bool()) is_truthy = condition.as_bool();
                
                if (!is_truthy)
                {
                    m_frames.back().ip += inst.operand;
                }
                break;
            }
            case OpCode::Loop:
            {
                m_frames.back().ip -= inst.operand;
                break;
            }
            case OpCode::Return:
            {
                Value result = pop(); // Return value
                
                u32 slots_offset = m_frames.back().slots_offset;
                m_frames.pop_back();
                
                if (m_frames.empty())
                {
                    return InterpretResult::Ok; // Top level script finished
                }
                
                // Pop locals, arguments, and the callee
                while (m_stack.size() > slots_offset)
                {
                    m_stack.pop_back();
                }
                
                push(result); // Put return value where the callee was
                break;
            }
            case OpCode::Call:
            {
                u32 arg_count = inst.operand;
                Value callee = peek(arg_count);
                
                if (callee.is_native_fn())
                {
                    std::vector<Value> args(arg_count);
                    for (int i = arg_count - 1; i >= 0; --i)
                    {
                        args[i] = pop();
                    }
                    pop(); // Pop callee
                    
                    NativeFn native = callee.as_native_fn();
                    Value result = native(args);
                    push(result);
                }
                else if (callee.is_function())
                {
                    std::shared_ptr<ObjFunction> function = callee.as_function();
                    if (arg_count != function->arity)
                    {
                        runtime_error("Expected different number of arguments.");
                        return InterpretResult::RuntimeError;
                    }
                    
                    u32 slots_offset = static_cast<u32>(m_stack.size()) - arg_count - 1;
                    m_frames.push_back(CallFrame{function, 0, slots_offset});
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
