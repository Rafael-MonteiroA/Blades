#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// vm.hpp — The Blades Virtual Machine
// ─────────────────────────────────────────────────────────────────────────────

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

#include "compiler/ir.hpp"
#include "backend/value.hpp"

namespace blades
{

enum class InterpretResult
{
    Ok,
    CompileError,
    RuntimeError,
    Yield
};

class VM
{
public:
    VM();
    InterpretResult interpret(std::shared_ptr<ObjFunction> function);
    InterpretResult resume(std::shared_ptr<ObjFiber> fiber);

    // Expose for testing
    const std::unordered_map<std::string, Value>& get_globals() const { return m_globals; }
    const std::vector<Value>& get_stack() const { return m_current_fiber->stack; }
    std::shared_ptr<ObjFiber> get_current_fiber() const { return m_current_fiber; }
    
    Value get_top() const { return m_current_fiber->stack.back(); }
    void pop_value() { pop(); }

    void define_native(const std::string& name, NativeFn function);
    void define_global(const std::string& name, Value value);
    
    // Calls a method on an instance synchronously from C++
    InterpretResult call_method(Value instance, const std::string& method_name, const std::vector<Value>& args);
    
    // Instantiates a class from globals and returns the instance
    Value instantiate(const std::string& class_name, const std::vector<Value>& args);

private:
    std::shared_ptr<ObjFiber> m_current_fiber;
    std::unordered_map<std::string, Value> m_globals;

    InterpretResult run();
    
    void push(Value value);
    Value pop();
    Value peek(int distance) const;
    
    std::shared_ptr<ObjUpvalue> capture_upvalue(u32 local_index);
    void close_upvalues(u32 last_index);
    
    void runtime_error(const char* message);
};

} // namespace blades
