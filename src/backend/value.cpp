#include "backend/value.hpp"
#include "compiler/ir.hpp"
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
    if (value.is_function()) return "<fn " + value.as_function()->name + ">";
    
    if (value.is_array())
    {
        std::string result = "[";
        auto arr = value.as_array();
        for (size_t i = 0; i < arr->elements.size(); ++i)
        {
            if (i > 0) result += ", ";
            if (arr->elements[i].is_string()) {
                result += "\"" + arr->elements[i].as_string() + "\"";
            } else {
                result += to_string(arr->elements[i]);
            }
        }
        result += "]";
        return result;
    }
    
    return "<unknown value>";
}

} // namespace blades
