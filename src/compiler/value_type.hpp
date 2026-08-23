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
    Int,
    Float,
    Bool,
    String,
    Any      // For dynamic/native functions
};

[[nodiscard]] const char* to_string(ValueType type);

} // namespace blades
