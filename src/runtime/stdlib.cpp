#include "runtime/stdlib.hpp"
#include <iostream>
#include <chrono>
#include <random>
#include <string>

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

void register_stdlib(VM& vm)
{
    vm.define_native("print", stdlib_print);
    vm.define_native("clock", stdlib_clock);
    vm.define_native("type_of", stdlib_typeof);
    vm.define_native("random", stdlib_random);
    vm.define_native("input", stdlib_input);
    vm.define_native("len", stdlib_len);
}

} // namespace blades
