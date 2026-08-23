#include "compiler/value_type.hpp"

namespace blades
{

const char* to_string(ValueType type)
{
    switch (type)
    {
        case ValueType::Unknown: return "Unknown";
        case ValueType::Void:    return "Void";
        case ValueType::Int:     return "Int";
        case ValueType::Float:   return "Float";
        case ValueType::Bool:    return "Bool";
        case ValueType::String:  return "String";
        case ValueType::Any:     return "Any";
    }
    return "Unknown";
}

} // namespace blades
