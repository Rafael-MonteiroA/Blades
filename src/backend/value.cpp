#include "backend/value.hpp"
#include <sstream>

namespace blades
{

std::string to_string(const Value& value)
{
    if (value.is_nil()) return "nil";
    if (value.is_int()) return std::to_string(value.as_int());
    if (value.is_double())
    {
        std::ostringstream oss;
        oss << value.as_double();
        return oss.str();
    }
    if (value.is_bool()) return value.as_bool() ? "true" : "false";
    if (value.is_string()) return value.as_string();
    if (value.is_native_fn()) return "<native fn>";
    
    return "<unknown value>";
}

} // namespace blades
