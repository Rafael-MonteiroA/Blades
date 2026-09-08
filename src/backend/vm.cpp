#include "backend/vm.hpp"

#include <cmath>
#include <iostream>
#include <sstream>

#include "runtime/stdlib.hpp"

namespace blades
{

VM::VM()
{
    m_current_fiber = std::make_shared<ObjFiber>();
}

void VM::push(Value value)
{
    m_current_fiber->stack.push_back(std::move(value));
}

Value VM::pop()
{
    Value value = std::move(m_current_fiber->stack.back());
    m_current_fiber->stack.pop_back();
    return value;
}

Value VM::peek(int distance) const
{
    return m_current_fiber->stack[m_current_fiber->stack.size() - 1 - distance];
}

std::shared_ptr<ObjUpvalue> VM::capture_upvalue(u32 local_index)
{
    std::shared_ptr<ObjUpvalue> prev_upvalue = nullptr;
    auto upvalue = m_current_fiber->open_upvalues.empty() ? nullptr : m_current_fiber->open_upvalues.front();
    
    // We keep m_current_fiber->open_upvalues sorted by location (descending, wait, or we just do a linear search since it's a simple vector).
    // Actually, searching linearly and just adding it is easiest, or sorted.
    for (auto it = m_current_fiber->open_upvalues.begin(); it != m_current_fiber->open_upvalues.end(); ++it)
    {
        if ((*it)->location == local_index)
        {
            return *it;
        }
    }
    
    auto created = std::make_shared<ObjUpvalue>();
    created->location = local_index;
    created->is_closed = false;
    m_current_fiber->open_upvalues.push_back(created);
    return created;
}

void VM::close_upvalues(u32 last_index)
{
    auto it = m_current_fiber->open_upvalues.begin();
    while (it != m_current_fiber->open_upvalues.end())
    {
        if ((*it)->location >= last_index)
        {
            (*it)->closed = m_current_fiber->stack[(*it)->location];
            (*it)->is_closed = true;
            it = m_current_fiber->open_upvalues.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void VM::runtime_error(const char* message)
{
    std::cerr << "Runtime Error: " << message << "\n";
    for (int i = static_cast<int>(m_current_fiber->frames.size()) - 1; i >= 0; i--)
    {
        CallFrame& frame = m_current_fiber->frames[i];
        auto function = frame.closure->function;
        u32 instruction = frame.ip > 0 ? frame.ip - 1 : 0;
        if (instruction < function->chunk.code.size())
        {
            std::cerr << "[";
            if (!function->source_name.empty()) std::cerr << function->source_name << ':';
            std::cerr << function->chunk.code[instruction].source_line << "] in ";
            if (function->name.empty()) std::cerr << "script\n";
            else std::cerr << function->name << "()\n";
        }
    }
}

void VM::define_native(const std::string& name, NativeFn function)
{
    m_globals[name] = Value(function);
}

void VM::define_global(const std::string& name, Value value)
{
    m_globals[name] = std::move(value);
}

InterpretResult VM::interpret(std::shared_ptr<ObjFunction> function)
{
    m_current_fiber->frames.clear();
    auto closure = std::make_shared<ObjClosure>();
    closure->function = function;
    // The top-level script is a function. We push it to the stack first so it can be popped on return.
    push(Value(closure));
    m_current_fiber->frames.push_back(CallFrame{closure, 0, 0}); // slots_offset is 0, callee at 0
    return run();
}

InterpretResult VM::resume(std::shared_ptr<ObjFiber> fiber)
{
    m_current_fiber = fiber;
    return run();
}

InterpretResult VM::call_method(Value instance, const std::string& method_name, const std::vector<Value>& args)
{
    if (!instance.is_instance()) return InterpretResult::RuntimeError;
    auto inst = instance.as_instance().get();
    
    auto method_it = inst->klass->methods.find(method_name);
    if (method_it == inst->klass->methods.end()) return InterpretResult::RuntimeError; // Method not found
    
    Value method = method_it->second;
    if (!method.is_closure()) return InterpretResult::RuntimeError;
    
    // We push the instance (which acts as the "this" receiver/callee)
    push(instance);
    // Push all arguments
    for (const auto& arg : args) {
        push(arg);
    }
    
    // Push CallFrame
    
    // Usually, callframe slots_offset is stack.size() - arg_count - 1
    u32 slots_offset = static_cast<u32>(m_current_fiber->stack.size() - args.size() - 1);
    
    // We make a copy of the shared_ptr to pass it to CallFrame safely
    auto closure_shared = method.as_closure();
    
    m_current_fiber->frames.push_back(CallFrame{closure_shared, 0, slots_offset});
    
    return run();
}

Value VM::instantiate(const std::string& class_name, const std::vector<Value>& args)
{
    auto it = m_globals.find(class_name);
    if (it == m_globals.end()) return Value(Nil{});
    
    Value klass_val = it->second;
    if (!klass_val.is_class()) return Value(Nil{});
    
    // Create the instance
    auto klass = klass_val.as_class().get();
    auto instance = std::make_shared<ObjInstance>();
    instance->klass = klass_val.as_class();
    Value inst_val(instance);
    
    // Call init() if it exists
    auto init_it = klass->methods.find("init");
    if (init_it != klass->methods.end())
    {
        call_method(inst_val, "init", args);
    }
    
    return inst_val;
}

InterpretResult VM::run()
{
    #define READ_INSTRUCTION() (m_current_fiber->frames.back().closure->function->chunk.code[m_current_fiber->frames.back().ip++])
    #define READ_CONSTANT(operand) (m_current_fiber->frames.back().closure->function->chunk.constants[operand])
    
    while (m_current_fiber->frames.back().ip < m_current_fiber->frames.back().closure->function->chunk.code.size())
    {
        Instruction inst = READ_INSTRUCTION();
        
        switch (inst.op)
        {
            case OpCode::Yield:
            {
                // We just return InterpretResult::Yield. 
                // The current instruction is already advanced in ip++, so it will resume on the NEXT instruction.
                return InterpretResult::Yield;
            }
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
                else if (a.is_string() || b.is_string())
                {
                    push(Value(to_string(a) + to_string(b)));
                }
                else if (a.is_vec2() && b.is_vec2())
                {
                    push(Value(ObjVec2{a.as_vec2().x + b.as_vec2().x, a.as_vec2().y + b.as_vec2().y}));
                }
                else if (a.is_vec3() && b.is_vec3())
                {
                    push(Value(ObjVec3{a.as_vec3().x + b.as_vec3().x, a.as_vec3().y + b.as_vec3().y, a.as_vec3().z + b.as_vec3().z}));
                }
                else
                {
                    runtime_error("Operands must be two numbers, strings, or vectors.");
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
                else if (a.is_vec2() && b.is_vec2())
                {
                    push(Value(ObjVec2{a.as_vec2().x - b.as_vec2().x, a.as_vec2().y - b.as_vec2().y}));
                }
                else if (a.is_vec3() && b.is_vec3())
                {
                    push(Value(ObjVec3{a.as_vec3().x - b.as_vec3().x, a.as_vec3().y - b.as_vec3().y, a.as_vec3().z - b.as_vec3().z}));
                }
                else
                {
                    runtime_error("Operands must be numbers or vectors.");
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
                else if (a.is_vec2() && b.is_number())
                {
                    float s = (float)b.as_number();
                    push(Value(ObjVec2{a.as_vec2().x * s, a.as_vec2().y * s}));
                }
                else if (a.is_number() && b.is_vec2())
                {
                    float s = (float)a.as_number();
                    push(Value(ObjVec2{b.as_vec2().x * s, b.as_vec2().y * s}));
                }
                else if (a.is_vec3() && b.is_number())
                {
                    float s = (float)b.as_number();
                    push(Value(ObjVec3{a.as_vec3().x * s, a.as_vec3().y * s, a.as_vec3().z * s}));
                }
                else if (a.is_number() && b.is_vec3())
                {
                    float s = (float)a.as_number();
                    push(Value(ObjVec3{b.as_vec3().x * s, b.as_vec3().y * s, b.as_vec3().z * s}));
                }
                else
                {
                    runtime_error("Operands must be numbers or vector/scalar.");
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
                else if (a.is_vec2() && b.is_number())
                {
                    float s = (float)b.as_number();
                    if (s == 0) { runtime_error("Division by zero."); return InterpretResult::RuntimeError; }
                    push(Value(ObjVec2{a.as_vec2().x / s, a.as_vec2().y / s}));
                }
                else if (a.is_vec3() && b.is_number())
                {
                    float s = (float)b.as_number();
                    if (s == 0) { runtime_error("Division by zero."); return InterpretResult::RuntimeError; }
                    push(Value(ObjVec3{a.as_vec3().x / s, a.as_vec3().y / s, a.as_vec3().z / s}));
                }
                else
                {
                    runtime_error("Operands must be numbers or vector/scalar.");
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OpCode::Modulo:
            {
                Value b = pop();
                Value a = pop();
                if (a.is_int() && b.is_int())
                {
                    if (b.as_int() == 0)
                    {
                        runtime_error("Modulo by zero.");
                        return InterpretResult::RuntimeError;
                    }
                    push(Value(a.as_int() % b.as_int()));
                }
                else if (a.is_number() && b.is_number())
                {
                    // Float modulo via fmod
                    if (b.as_number() == 0)
                    {
                        runtime_error("Modulo by zero.");
                        return InterpretResult::RuntimeError;
                    }
                    push(Value(std::fmod(a.as_number(), b.as_number())));
                }
                else
                {
                    runtime_error("Operands for '%' must be numbers.");
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
            case OpCode::BitAnd:
            {
                Value b = pop();
                Value a = pop();
                if (a.is_int() && b.is_int()) push(Value(a.as_int() & b.as_int()));
                else { runtime_error("Operands for bitwise AND must be integers."); return InterpretResult::RuntimeError; }
                break;
            }
            case OpCode::BitOr:
            {
                Value b = pop();
                Value a = pop();
                if (a.is_int() && b.is_int()) push(Value(a.as_int() | b.as_int()));
                else { runtime_error("Operands for bitwise OR must be integers."); return InterpretResult::RuntimeError; }
                break;
            }
            case OpCode::BitXor:
            {
                Value b = pop();
                Value a = pop();
                if (a.is_int() && b.is_int()) push(Value(a.as_int() ^ b.as_int()));
                else { runtime_error("Operands for bitwise XOR must be integers."); return InterpretResult::RuntimeError; }
                break;
            }
            case OpCode::BitNot:
            {
                Value v = pop();
                if (v.is_int()) push(Value(~v.as_int()));
                else { runtime_error("Operand for bitwise NOT must be an integer."); return InterpretResult::RuntimeError; }
                break;
            }
            case OpCode::ShiftLeft:
            {
                Value b = pop();
                Value a = pop();
                if (a.is_int() && b.is_int()) push(Value(a.as_int() << b.as_int()));
                else { runtime_error("Operands for shift left must be integers."); return InterpretResult::RuntimeError; }
                break;
            }
            case OpCode::ShiftRight:
            {
                Value b = pop();
                Value a = pop();
                if (a.is_int() && b.is_int()) push(Value(a.as_int() >> b.as_int()));
                else { runtime_error("Operands for shift right must be integers."); return InterpretResult::RuntimeError; }
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
                push(m_current_fiber->stack[m_current_fiber->frames.back().slots_offset + inst.operand]);
                break;
            }
            case OpCode::SetLocal:
            {
                u32 slot = m_current_fiber->frames.back().slots_offset + inst.operand;
                m_current_fiber->stack[slot] = peek(0);
                break;
            }
            case OpCode::BuildList:
            {
                u32 count = inst.operand;
                auto arr = std::make_shared<ObjArray>();
                arr->elements.reserve(count);
                for (u32 i = 0; i < count; ++i)
                {
                    arr->elements.push_back(m_current_fiber->stack[m_current_fiber->stack.size() - count + i]);
                }
                for (u32 i = 0; i < count; ++i)
                {
                    m_current_fiber->stack.pop_back();
                }
                push(Value(arr));
                break;
            }
            case OpCode::BuildDict:
            {
                u32 count = inst.operand;
                auto dict = std::make_shared<ObjDict>();
                // The stack has key1, val1, key2, val2, ...
                // Since we pop, we get val_n, key_n, val_n-1, key_n-1...
                for (u32 i = 0; i < count; ++i)
                {
                    Value value = pop();
                    Value key = pop();
                    if (!key.is_string())
                    {
                        runtime_error("Dictionary keys must be strings.");
                        return InterpretResult::RuntimeError;
                    }
                    dict->elements[key.as_string()] = value;
                }
                push(Value(dict));
                break;
            }
            case OpCode::GetSubscript:
            {
                Value index = pop();
                Value object = pop();

                if (object.is_array())
                {
                    if (!index.is_int())
                    {
                        runtime_error("Array index must be an integer.");
                        return InterpretResult::RuntimeError;
                    }
                    int64_t idx = index.as_int();
                    auto arr = object.as_array();
                    if (idx < 0 || idx >= static_cast<int64_t>(arr->elements.size()))
                    {
                        runtime_error("Index out of bounds.");
                        return InterpretResult::RuntimeError;
                    }
                    push(arr->elements[static_cast<size_t>(idx)]);
                }
                else if (object.is_dict())
                {
                    if (!index.is_string())
                    {
                        runtime_error("Dictionary key must be a string.");
                        return InterpretResult::RuntimeError;
                    }
                    auto dict = object.as_dict();
                    auto it = dict->elements.find(index.as_string());
                    if (it == dict->elements.end())
                    {
                        push(Value(Nil{})); // Return nil if key not found
                    }
                    else
                    {
                        push(it->second);
                    }
                }
                else if (object.is_string())
                {
                    if (!index.is_int())
                    {
                        runtime_error("String index must be an integer.");
                        return InterpretResult::RuntimeError;
                    }
                    int64_t idx = index.as_int();
                    const std::string& str = object.as_string();
                    if (idx < 0 || idx >= static_cast<int64_t>(str.size()))
                    {
                        runtime_error("String index out of bounds.");
                        return InterpretResult::RuntimeError;
                    }
                    push(Value(std::string(1, str[static_cast<size_t>(idx)])));
                }
                else
                {
                    runtime_error("Object is not subscriptable.");
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OpCode::SetSubscript:
            {
                Value value = pop();
                Value index = pop();
                Value object = pop();

                if (object.is_array())
                {
                    if (!index.is_int())
                    {
                        runtime_error("Array index must be an integer.");
                        return InterpretResult::RuntimeError;
                    }
                    int64_t idx = index.as_int();
                    auto arr = object.as_array();
                    if (idx < 0 || idx >= static_cast<int64_t>(arr->elements.size()))
                    {
                        runtime_error("Index out of bounds.");
                        return InterpretResult::RuntimeError;
                    }
                    arr->elements[static_cast<size_t>(idx)] = value;
                    push(value);
                }
                else if (object.is_dict())
                {
                    if (!index.is_string())
                    {
                        runtime_error("Dictionary key must be a string.");
                        return InterpretResult::RuntimeError;
                    }
                    auto dict = object.as_dict();
                    dict->elements[index.as_string()] = value;
                    push(value);
                }
                else
                {
                    runtime_error("Object is not subscriptable.");
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OpCode::GetProperty:
            {
                Value object = pop();
                std::string name = READ_CONSTANT(inst.operand).as_string();
                
                if (object.is_dict())
                {
                    auto dict = object.as_dict();
                    auto it = dict->elements.find(name);
                    if (it == dict->elements.end()) push(Value(Nil{}));
                    else push(it->second);
                }
                else if (object.is_instance())
                {
                    auto instance = object.as_instance();
                    auto it = instance->fields.find(name);
                    if (it != instance->fields.end())
                    {
                        push(it->second);
                    }
                    else
                    {
                        // Bind method if it exists
                        auto method_it = instance->klass->methods.find(name);
                        if (method_it != instance->klass->methods.end())
                        {
                            auto bound = std::make_shared<ObjBoundMethod>();
                            bound->receiver = object;
                            bound->method = method_it->second.as_closure();
                            push(Value(bound));
                        }
                        else
                        {
                            push(Value(Nil{}));
                        }
                    }
                }
                else if (object.is_array())
                {
                    if (name == "length")
                    {
                        auto arr = object.as_array();
                        push(Value(static_cast<int64_t>(arr->elements.size())));
                    }
                    else
                    {
                        runtime_error(("Undefined property '" + name + "' on array.").c_str());
                        return InterpretResult::RuntimeError;
                    }
                }
                else if (object.is_vec2())
                {
                    if (name == "x") push(Value((double)object.as_vec2().x));
                    else if (name == "y") push(Value((double)object.as_vec2().y));
                    else if (name == "length") 
                    {
                        float x = object.as_vec2().x;
                        float y = object.as_vec2().y;
                        push(Value((double)std::sqrt(x*x + y*y)));
                    }
                    else if (name == "normalized")
                    {
                        float x = object.as_vec2().x;
                        float y = object.as_vec2().y;
                        float len = std::sqrt(x*x + y*y);
                        if (len == 0.0f) push(Value(ObjVec2{0.0f, 0.0f}));
                        else push(Value(ObjVec2{x/len, y/len}));
                    }
                    else { runtime_error("Invalid property for Vec2."); return InterpretResult::RuntimeError; }
                }
                else if (object.is_vec3())
                {
                    if (name == "x") push(Value((double)object.as_vec3().x));
                    else if (name == "y") push(Value((double)object.as_vec3().y));
                    else if (name == "z") push(Value((double)object.as_vec3().z));
                    else if (name == "length")
                    {
                        float x = object.as_vec3().x;
                        float y = object.as_vec3().y;
                        float z = object.as_vec3().z;
                        push(Value((double)std::sqrt(x*x + y*y + z*z)));
                    }
                    else if (name == "normalized")
                    {
                        float x = object.as_vec3().x;
                        float y = object.as_vec3().y;
                        float z = object.as_vec3().z;
                        float len = std::sqrt(x*x + y*y + z*z);
                        if (len == 0.0f) push(Value(ObjVec3{0.0f, 0.0f, 0.0f}));
                        else push(Value(ObjVec3{x/len, y/len, z/len}));
                    }
                    else { runtime_error("Invalid property for Vec3."); return InterpretResult::RuntimeError; }
                }
                else if (object.is_color())
                {
                    if (name == "r") push(Value((double)object.as_color().r));
                    else if (name == "g") push(Value((double)object.as_color().g));
                    else if (name == "b") push(Value((double)object.as_color().b));
                    else if (name == "a") push(Value((double)object.as_color().a));
                    else { runtime_error("Invalid property for Color."); return InterpretResult::RuntimeError; }
                }
                else
                {
                    runtime_error("Only dictionaries, instances, and vectors have properties.");
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OpCode::SetProperty:
            {
                Value value = pop();
                Value object = pop();
                std::string name = READ_CONSTANT(inst.operand).as_string();
                
                if (object.is_dict())
                {
                    auto dict = object.as_dict();
                    dict->elements[name] = value;
                    push(value);
                }
                else if (object.is_instance())
                {
                    auto instance = object.as_instance();
                    instance->fields[name] = value;
                    push(value);
                }
                else if (object.is_vec2() || object.is_vec3() || object.is_color())
                {
                    runtime_error("Cannot mutate value types (Vector/Color) directly. Reassign the entire value.");
                    return InterpretResult::RuntimeError;
                }
                else
                {
                    runtime_error("Only dictionaries and instances have mutable properties.");
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OpCode::Class:
            {
                std::string name = READ_CONSTANT(inst.operand).as_string();
                auto klass = std::make_shared<ObjClass>();
                klass->name = name;
                push(Value(klass));
                break;
            }
            case OpCode::Method:
            {
                std::string name = READ_CONSTANT(inst.operand).as_string();
                Value method = pop();
                Value klass_val = pop();
                if (klass_val.is_class())
                {
                    klass_val.as_class()->methods[name] = method;
                }
                break;
            }
            case OpCode::Inherit:
            {
                Value subclass_val = pop();
                Value superclass_val = pop();
                
                if (!superclass_val.is_class()) {
                    runtime_error("Superclass must be a class.");
                    return InterpretResult::RuntimeError;
                }
                
                auto subclass = subclass_val.as_class();
                auto superclass = superclass_val.as_class();
                
                subclass->superclass = superclass;
                for (const auto& [name, method] : superclass->methods) {
                    subclass->methods[name] = method;
                }
                
                break;
            }
            case OpCode::GetSuper:
            {
                std::string name = READ_CONSTANT(inst.operand).as_string();
                Value superclass_val = pop();
                Value instance_val = pop();
                
                if (!superclass_val.is_class()) {
                    runtime_error("Superclass must be a class.");
                    return InterpretResult::RuntimeError;
                }
                
                auto superclass = superclass_val.as_class();
                auto it = superclass->methods.find(name);
                if (it == superclass->methods.end()) {
                    runtime_error(("Undefined property '" + name + "'.").c_str());
                    return InterpretResult::RuntimeError;
                }
                
                auto bound = std::make_shared<ObjBoundMethod>();
                bound->receiver = instance_val;
                bound->method = it->second.as_closure();
                push(Value(bound));
                break;
            }
            case OpCode::Pop:
            {
                pop();
                break;
            }
            case OpCode::Dup:
            {
                push(peek(0));
                break;
            }
            case OpCode::Closure:
            {
                auto function = READ_CONSTANT(inst.operand).as_function();
                auto closure = std::make_shared<ObjClosure>();
                closure->function = function;
                for (const auto& upvalue : function->captured_upvalues)
                {
                    if (upvalue.is_local)
                    {
                        closure->upvalues.push_back(capture_upvalue(m_current_fiber->frames.back().slots_offset + upvalue.index));
                    }
                    else
                    {
                        closure->upvalues.push_back(m_current_fiber->frames.back().closure->upvalues[upvalue.index]);
                    }
                }
                push(Value(closure));
                break;
            }
            case OpCode::GetUpvalue:
            {
                auto upvalue = m_current_fiber->frames.back().closure->upvalues[inst.operand];
                if (upvalue->is_closed) push(upvalue->closed);
                else push(m_current_fiber->stack[upvalue->location]);
                break;
            }
            case OpCode::SetUpvalue:
            {
                auto upvalue = m_current_fiber->frames.back().closure->upvalues[inst.operand];
                if (upvalue->is_closed) upvalue->closed = peek(0);
                else m_current_fiber->stack[upvalue->location] = peek(0);
                break;
            }
            case OpCode::CloseUpvalue:
            {
                close_upvalues(static_cast<u32>(m_current_fiber->stack.size() - 1));
                pop();
                break;
            }
            case OpCode::Jump:
            {
                m_current_fiber->frames.back().ip += inst.operand;
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
                    m_current_fiber->frames.back().ip += inst.operand;
                }
                break;
            }
            case OpCode::Loop:
            {
                m_current_fiber->frames.back().ip -= inst.operand;
                break;
            }
            case OpCode::Return:
            {
                Value result = pop(); // Return value
                
                u32 slots_offset = m_current_fiber->frames.back().slots_offset;
                bool is_init = (m_current_fiber->frames.back().closure->function->name == "init");
                
                close_upvalues(slots_offset);
                
                m_current_fiber->frames.pop_back();
                
                if (m_current_fiber->frames.empty())
                {
                    return InterpretResult::Ok; // Top level script finished
                }
                
                Value callee_or_this = m_current_fiber->stack[slots_offset];
                
                // Pop locals, arguments, and the callee
                while (m_current_fiber->stack.size() > slots_offset)
                {
                    m_current_fiber->stack.pop_back();
                }
                
                if (is_init) {
                    push(callee_or_this); // Return 'this' instead of result
                } else {
                    push(result); // Put return value where the callee was
                }
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
                    try
                    {
                        Value result = native(args);
                        if (result.is_native_error())
                        {
                            runtime_error(result.as_native_error().message.c_str());
                            return InterpretResult::RuntimeError;
                        }
                        push(result);
                    }
                    catch (const std::exception& error)
                    {
                        runtime_error(error.what());
                        return InterpretResult::RuntimeError;
                    }
                }
                else if (callee.is_closure())
                {
                    std::shared_ptr<ObjClosure> closure = callee.as_closure();
                    if (arg_count != closure->function->arity)
                    {
                        runtime_error("Expected different number of arguments.");
                        return InterpretResult::RuntimeError;
                    }
                    
                    u32 slots_offset = static_cast<u32>(m_current_fiber->stack.size()) - arg_count - 1;
                    m_current_fiber->frames.push_back(CallFrame{closure, 0, slots_offset});
                }
                else if (callee.is_class())
                {
                    std::shared_ptr<ObjClass> klass = callee.as_class();
                    auto instance = std::make_shared<ObjInstance>();
                    instance->klass = klass;
                    
                    // Call 'init' if it exists
                    auto it = klass->methods.find("init");
                    if (it != klass->methods.end())
                    {
                        // Replace the callee (the class) with the instance so 'this' is in slot 0
                        m_current_fiber->stack[m_current_fiber->stack.size() - arg_count - 1] = Value(instance);
                        
                        std::shared_ptr<ObjClosure> init_closure = it->second.as_closure();
                        if (arg_count != init_closure->function->arity)
                        {
                            runtime_error("Expected different number of arguments for init.");
                            return InterpretResult::RuntimeError;
                        }
                        
                        u32 slots_offset = static_cast<u32>(m_current_fiber->stack.size()) - arg_count - 1;
                        m_current_fiber->frames.push_back(CallFrame{init_closure, 0, slots_offset});
                    }
                    else if (arg_count != 0)
                    {
                        runtime_error("Expected 0 arguments since class has no init method.");
                        return InterpretResult::RuntimeError;
                    }
                    else
                    {
                        // Just replace callee with instance and return it (no frame needed).
                        m_current_fiber->stack[m_current_fiber->stack.size() - 1] = Value(instance);
                    }
                }
                else if (callee.is_bound_method())
                {
                    std::shared_ptr<ObjBoundMethod> bound = callee.as_bound_method();
                    if (arg_count != bound->method->function->arity)
                    {
                        runtime_error("Expected different number of arguments.");
                        return InterpretResult::RuntimeError;
                    }
                    
                    // Replace the bound method with the receiver
                    m_current_fiber->stack[m_current_fiber->stack.size() - arg_count - 1] = bound->receiver;
                    
                    u32 slots_offset = static_cast<u32>(m_current_fiber->stack.size()) - arg_count - 1;
                    m_current_fiber->frames.push_back(CallFrame{bound->method, 0, slots_offset});
                }
                else
                {
                    runtime_error("Can only call functions and classes.");
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OpCode::Break:
            case OpCode::Continue:
            {
                runtime_error("Invalid loop opcode in bytecode.");
                return InterpretResult::RuntimeError;
            }
        }
    }
    
    #undef READ_INSTRUCTION
    #undef READ_CONSTANT
    
    return InterpretResult::Ok;
}

} // namespace blades

