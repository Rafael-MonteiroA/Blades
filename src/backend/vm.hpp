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
    RuntimeError
};

struct CallFrame
{
    std::shared_ptr<ObjFunction> function;
    u32 ip = 0;
    u32 slots_offset = 0;
};

class VM
{
public:
    InterpretResult interpret(std::shared_ptr<ObjFunction> function);

    // Expose for testing
    const std::unordered_map<std::string, Value>& get_globals() const { return m_globals; }
    const std::vector<Value>& get_stack() const { return m_stack; }

    void define_native(const std::string& name, NativeFn function);

private:
    std::vector<CallFrame> m_frames;
    std::vector<Value> m_stack;
    std::unordered_map<std::string, Value> m_globals;

    InterpretResult run();
    
    void push(Value value);
    Value pop();
    Value peek(int distance) const;
    
    void runtime_error(const char* message);
};

} // namespace blades
