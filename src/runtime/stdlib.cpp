#include "runtime/stdlib.hpp"
#include <iostream>
#include <chrono>

namespace blades
{

static Value stdlib_print(const std::vector<Value>& args)
{
    for (size_t i = 0; i < args.size(); ++i)
    {
        std::cout << to_string(args[i]);
        if (i < args.size() - 1) std::cout << " ";
    }
    std::cout << "\n";
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
    if (val.is_native_fn()) return Value("native_fn");
    
    return Value("unknown");
}

void register_stdlib(VM& vm)
{
    vm.define_native("print", stdlib_print);
    vm.define_native("clock", stdlib_clock);
    vm.define_native("type_of", stdlib_typeof);
}

} // namespace blades
