#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// value_type.hpp — Type system representation for Blades
// ─────────────────────────────────────────────────────────────────────────────

#include <string>

namespace blades
{

enum class ValueType
{
    Unknown, // For expressions that failed type checking or haven't been checked
    Void,    // For statements or functions that return nothing
    Nil,     // Nil type
    Int,
    Float,
    Bool,
    String,
    Array,
    Dict,
    Function,
    Closure,
    Class,
    Instance,
    NativeFn,
    Vec2,
    Vec3,
    Color,
    Fiber,
    UserData,
    Any      // For dynamic/native functions
};

[[nodiscard]] const char* to_string(ValueType type);

} // namespace blades
