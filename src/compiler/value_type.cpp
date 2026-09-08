#include "compiler/value_type.hpp"

namespace blades
{

const char* to_string(ValueType type)
{
    switch (type)
    {
        case ValueType::Unknown:  return "Unknown";
        case ValueType::Void:     return "Void";
        case ValueType::Nil:      return "Nil";
        case ValueType::Int:      return "Int";
        case ValueType::Float:    return "Float";
        case ValueType::Number:   return "Number";
        case ValueType::Bool:     return "Bool";
        case ValueType::String:   return "String";
        case ValueType::Array:    return "Array";
        case ValueType::Dict:     return "Dict";
        case ValueType::Function: return "Function";
        case ValueType::Closure:  return "Closure";
        case ValueType::Class:    return "Class";
        case ValueType::Instance: return "Instance";
        case ValueType::NativeFn: return "NativeFn";
        case ValueType::Vec2:     return "Vec2";
        case ValueType::Vec3:     return "Vec3";
        case ValueType::Color:    return "Color";
        case ValueType::Fiber:    return "Fiber";
        case ValueType::UserData: return "UserData";
        case ValueType::Any:      return "Any";
    }
    return "Unknown";
}

} // namespace blades
