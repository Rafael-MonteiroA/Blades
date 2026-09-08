#include "backend/value.hpp"
#include "compiler/ir.hpp"
#include <sstream>

namespace blades
{

std::string to_string(const Value& value)
{
    if (value.is_nil()) return "nil";
    if (value.is_native_error()) return "<native error: " + value.as_native_error().message + ">";
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
    if (value.is_closure()) {
        std::string name = value.as_closure()->function->name;
        if (name.empty()) return "<lambda>";
        return "<fn " + name + ">";
    }
    
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
    
    if (value.is_dict())
    {
        std::string result = "{";
        auto dict = value.as_dict();
        bool first = true;
        for (const auto& [k, v] : dict->elements)
        {
            if (!first) result += ", ";
            first = false;
            result += k + ": ";
            if (v.is_string()) result += "\"" + v.as_string() + "\"";
            else result += to_string(v);
        }
        result += "}";
        return result;
    }
    
    if (value.is_class()) return "<class " + value.as_class()->name + ">";
    if (value.is_instance()) return "<instance " + value.as_instance()->klass->name + ">";
    if (value.is_bound_method()) return "<bound method " + value.as_bound_method()->method->function->name + ">";
    if (value.is_user_data()) return "<userdata>";
    if (value.is_fiber()) return "<fiber>";

    if (value.is_vec2()) {
        auto v = value.as_vec2();
        std::ostringstream oss; oss << "<Vec2(" << v.x << ", " << v.y << ")>";
        return oss.str();
    }
    if (value.is_vec3()) {
        auto v = value.as_vec3();
        std::ostringstream oss; oss << "<Vec3(" << v.x << ", " << v.y << ", " << v.z << ")>";
        return oss.str();
    }
    if (value.is_color()) {
        auto c = value.as_color();
        std::ostringstream oss; oss << "<Color(" << (int)c.r << ", " << (int)c.g << ", " << (int)c.b << ", " << (int)c.a << ")>";
        return oss.str();
    }

    return "<unknown value>";
}

} // namespace blades
