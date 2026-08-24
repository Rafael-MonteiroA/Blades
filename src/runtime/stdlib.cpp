#include "runtime/stdlib.hpp"
#include <iostream>
#include <chrono>
#include <random>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>

namespace blades
{

static Value stdlib_print(const std::vector<Value>& args)
{
    for (size_t i = 0; i < args.size(); ++i)
    {
        std::cout << to_string(args[i]);
        if (i < args.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
    return Value(Nil{});
}

static Value stdlib_array_push(const std::vector<Value>& args)
{
    if (args.size() == 2 && args[0].is_array())
    {
        auto arr = args[0].as_array();
        arr->elements.push_back(args[1]);
        return Value(Nil{});
    }
    return Value(Nil{});
}

static Value stdlib_array_pop(const std::vector<Value>& args)
{
    if (args.size() == 1 && args[0].is_array())
    {
        auto arr = args[0].as_array();
        if (!arr->elements.empty())
        {
            Value val = arr->elements.back();
            arr->elements.pop_back();
            return val;
        }
    }
    return Value(Nil{});
}

static Value stdlib_clock(const std::vector<Value>& /*args*/)
{
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return Value(std::chrono::duration_cast<std::chrono::duration<double>>(now).count());
}

static Value stdlib_typeof(const std::vector<Value>& args)
{
    if (args.empty()) return Value("nil");
    
    const Value& val = args[0];
    if (val.is_nil()) return Value("nil");
    if (val.is_int()) return Value("int");
    if (val.is_double()) return Value("double");
    if (val.is_bool()) return Value("bool");
    if (val.is_string()) return Value("string");
    if (val.is_function() || val.is_native_fn() || val.is_bound_method()) return Value("function");
    if (val.is_array()) return Value("array");
    if (val.is_dict()) return Value("dict");
    if (val.is_class()) return Value("class");
    if (val.is_instance()) return Value("instance");
    
    return Value("unknown");
}

static Value stdlib_random(const std::vector<Value>& args)
{
    static std::mt19937 rng(std::random_device{}());
    
    if (args.empty())
    {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return Value(dist(rng));
    }
    
    if (args.size() == 2 && args[0].is_int() && args[1].is_int())
    {
        std::uniform_int_distribution<int> dist(args[0].as_int(), args[1].as_int());
        return Value(dist(rng));
    }
    
    return Value(Nil{}); // Should throw a runtime error in a robust implementation
}

static Value stdlib_input(const std::vector<Value>& args)
{
    if (!args.empty() && args[0].is_string())
    {
        std::cout << args[0].as_string();
    }
    
    std::string line;
    std::getline(std::cin, line);
    return Value(line);
}

static Value stdlib_len(const std::vector<Value>& args)
{
    if (args.empty()) return Value(0);
    
    if (args[0].is_string())
    {
        return Value(static_cast<int>(args[0].as_string().length()));
    }
    else if (args[0].is_array())
    {
        return Value(static_cast<int>(args[0].as_array()->elements.size()));
    }
    else if (args[0].is_dict())
    {
        return Value(static_cast<int>(args[0].as_dict()->elements.size()));
    }
    
    return Value(0);
}

static Value stdlib_vec2(const std::vector<Value>& args)
{
    float x = args.size() > 0 ? (float)args[0].as_number() : 0.0f;
    float y = args.size() > 1 ? (float)args[1].as_number() : 0.0f;
    return Value(ObjVec2{x, y});
}

static Value stdlib_vec3(const std::vector<Value>& args)
{
    float x = args.size() > 0 ? (float)args[0].as_number() : 0.0f;
    float y = args.size() > 1 ? (float)args[1].as_number() : 0.0f;
    float z = args.size() > 2 ? (float)args[2].as_number() : 0.0f;
    return Value(ObjVec3{x, y, z});
}

static Value stdlib_color(const std::vector<Value>& args)
{
    unsigned char r = args.size() > 0 ? (unsigned char)args[0].as_number() : 0;
    unsigned char g = args.size() > 1 ? (unsigned char)args[1].as_number() : 0;
    unsigned char b = args.size() > 2 ? (unsigned char)args[2].as_number() : 0;
    unsigned char a = args.size() > 3 ? (unsigned char)args[3].as_number() : 255;
    return Value(ObjColor{r, g, b, a});
}

static Value stdlib_sin(const std::vector<Value>& args)
{
    if (args.empty() || (!args[0].is_number())) return Value(0.0);
    return Value(std::sin(args[0].as_number()));
}

static Value stdlib_cos(const std::vector<Value>& args)
{
    if (args.empty() || (!args[0].is_number())) return Value(0.0);
    return Value(std::cos(args[0].as_number()));
}

static Value stdlib_tan(const std::vector<Value>& args)
{
    if (args.empty() || (!args[0].is_number())) return Value(0.0);
    return Value(std::tan(args[0].as_number()));
}

static Value stdlib_sqrt(const std::vector<Value>& args)
{
    if (args.empty() || !args[0].is_number()) return Value(0.0);
    return Value(std::sqrt(args[0].as_number()));
}

static Value stdlib_abs(const std::vector<Value>& args)
{
    if (args.empty() || !args[0].is_number()) return Value(0.0);
    return Value(std::abs(args[0].as_number()));
}

static Value stdlib_pow(const std::vector<Value>& args)
{
    if (args.size() < 2 || !args[0].is_number() || !args[1].is_number()) return Value(0.0);
    return Value(std::pow(args[0].as_number(), args[1].as_number()));
}

static Value stdlib_dot(const std::vector<Value>& args)
{
    if (args.size() < 2) return Value(0.0);
    if (args[0].is_vec3() && args[1].is_vec3())
    {
        return Value((double)(args[0].as_vec3().x * args[1].as_vec3().x + 
                              args[0].as_vec3().y * args[1].as_vec3().y + 
                              args[0].as_vec3().z * args[1].as_vec3().z));
    }
    if (args[0].is_vec2() && args[1].is_vec2())
    {
        return Value((double)(args[0].as_vec2().x * args[1].as_vec2().x + 
                              args[0].as_vec2().y * args[1].as_vec2().y));
    }
    return Value(0.0);
}

static Value stdlib_read_text(const std::vector<Value>& args)
{
    if (args.empty() || !args[0].is_string()) return Value("");
    std::ifstream file(args[0].as_string());
    if (!file.is_open()) return Value("");
    std::stringstream buffer;
    buffer << file.rdbuf();
    return Value(buffer.str());
}

static Value stdlib_write_text(const std::vector<Value>& args)
{
    if (args.size() < 2 || !args[0].is_string() || !args[1].is_string()) return Value(false);
    std::ofstream file(args[0].as_string());
    if (!file.is_open()) return Value(false);
    file << args[1].as_string();
    return Value(true);
}

// ---------------- FFI TEST ----------------
struct FakePhysicsBody {
    float x = 100.0f;
    float y = 50.0f;
};
static FakePhysicsBody g_fake_body;

static Value stdlib_get_fake_body(const std::vector<Value>& args)
{
    (void)args;
    auto ud = std::make_shared<ObjUserData>();
    ud->data = &g_fake_body;
    return Value(ud);
}

static Value stdlib_move_fake_body(const std::vector<Value>& args)
{
    if (args.size() < 3 || !args[0].is_user_data() || !args[1].is_number() || !args[2].is_number()) return Value(false);
    auto ud = args[0].as_user_data();
    FakePhysicsBody* body = static_cast<FakePhysicsBody*>(ud->data);
    body->x += static_cast<float>(args[1].as_number());
    body->y += static_cast<float>(args[2].as_number());
    return Value(true);
}

static Value stdlib_get_fake_body_x(const std::vector<Value>& args)
{
    if (args.empty() || !args[0].is_user_data()) return Value(0.0);
    FakePhysicsBody* body = static_cast<FakePhysicsBody*>(args[0].as_user_data()->data);
    return Value(body->x);
}
// ------------------------------------------

void register_stdlib(VM& vm)
{
    vm.define_native("print", stdlib_print);
    vm.define_native("clock", stdlib_clock);
    vm.define_native("type_of", stdlib_typeof);
    vm.define_native("random", stdlib_random);
    vm.define_native("input", stdlib_input);
    vm.define_native("len", stdlib_len);
    vm.define_native("vec2", stdlib_vec2);
    vm.define_native("vec3", stdlib_vec3);
    vm.define_native("color", stdlib_color);
    
    // Math
    vm.define_native("sin", stdlib_sin);
    vm.define_native("cos", stdlib_cos);
    vm.define_native("tan", stdlib_tan);
    vm.define_native("sqrt", stdlib_sqrt);
    vm.define_native("abs", stdlib_abs);
    vm.define_native("pow", stdlib_pow);
    vm.define_native("dot", stdlib_dot);
    
    // File I/O
    vm.define_native("read_text", stdlib_read_text);
    vm.define_native("write_text", stdlib_write_text);
    
    // FFI Test
    vm.define_native("get_fake_body", stdlib_get_fake_body);
    vm.define_native("move_fake_body", stdlib_move_fake_body);
    vm.define_native("get_fake_body_x", stdlib_get_fake_body_x);

    // Arrays
    vm.define_native("array_push", stdlib_array_push);
    vm.define_native("array_pop", stdlib_array_pop);
}

} // namespace blades
