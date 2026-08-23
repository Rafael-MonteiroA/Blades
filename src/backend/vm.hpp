#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// vm.hpp — The Blades Virtual Machine
// ─────────────────────────────────────────────────────────────────────────────

#include <vector>
#include <unordered_map>
#include <string>

#include "compiler/ir.hpp"
#include "backend/value.hpp"

namespace blades
{

enum class InterpretResult
{
    Ok,
    CompileError,
    RuntimeError
};

class VM
{
public:
    InterpretResult interpret(const IRChunk& chunk);

    // Expose for testing
    const std::unordered_map<std::string, Value>& get_globals() const { return m_globals; }
    const std::vector<Value>& get_stack() const { return m_stack; }

    void define_native(const std::string& name, NativeFn function);

private:
    const IRChunk* m_chunk = nullptr;
    u32 m_ip = 0;
    std::vector<Value> m_stack;
    std::unordered_map<std::string, Value> m_globals;

    InterpretResult run();
    
    void push(Value value);
    Value pop();
    Value peek(int distance) const;
    
    void runtime_error(const char* message);
};

} // namespace blades
