#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// value.hpp — Runtime value representation for the VM
// ─────────────────────────────────────────────────────────────────────────────

#include <variant>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>

#include "common/types.hpp"

namespace blades
{

// We use std::monostate to represent 'nil'
using Nil = std::monostate;

struct Value;
using NativeFn = Value(*)(const std::vector<Value>& args);

struct ObjFunction;

struct ObjUpvalue;
struct ObjClosure;

struct CallFrame
{
    std::shared_ptr<ObjClosure> closure;
    u32 ip = 0;
    u32 slots_offset = 0;
};

struct ObjFiber
{
    std::vector<CallFrame> frames;
    std::vector<struct Value> stack;
    std::vector<std::shared_ptr<ObjUpvalue>> open_upvalues;
};

struct ObjClosure
{
    std::shared_ptr<ObjFunction> function;
    std::vector<std::shared_ptr<ObjUpvalue>> upvalues;
};

struct ObjArray
{
    std::vector<Value> elements;
};

struct ObjDict
{
    std::unordered_map<std::string, Value> elements;
};

struct ObjClass
{
    std::string name;
    std::shared_ptr<ObjClass> superclass;
    std::unordered_map<std::string, Value> methods;
};

struct ObjInstance
{
    std::shared_ptr<ObjClass> klass;
    std::unordered_map<std::string, Value> fields;
};

struct ObjBoundMethod;

struct ObjUserData
{
    void* data;
};

struct ObjVec2 { 
    float x; float y; 
    bool operator==(const ObjVec2& o) const { return x == o.x && y == o.y; }
};
struct ObjVec3 { 
    float x; float y; float z; 
    bool operator==(const ObjVec3& o) const { return x == o.x && y == o.y && z == o.z; }
};
struct ObjColor { 
    unsigned char r; unsigned char g; unsigned char b; unsigned char a; 
    bool operator==(const ObjColor& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
};

struct Value
{
    std::variant<Nil, int64_t, double, bool, std::string, NativeFn, std::shared_ptr<ObjFunction>, std::shared_ptr<ObjClosure>, std::shared_ptr<ObjArray>, std::shared_ptr<ObjDict>, std::shared_ptr<ObjClass>, std::shared_ptr<ObjInstance>, std::shared_ptr<ObjBoundMethod>, std::shared_ptr<ObjUserData>, std::shared_ptr<ObjFiber>, ObjVec2, ObjVec3, ObjColor> data;

    // Constructors
    Value() : data(Nil{}) {}
    Value(Nil n) : data(n) {}
    Value(int i) : data(static_cast<int64_t>(i)) {}
    Value(int64_t i) : data(i) {}
    Value(double d) : data(d) {}
    Value(bool b) : data(b) {}
    Value(std::string s) : data(std::move(s)) {}
    Value(const char* s) : data(std::string(s)) {}
    Value(NativeFn fn) : data(fn) {}
    Value(std::shared_ptr<ObjFunction> fn) : data(std::move(fn)) {}
    Value(std::shared_ptr<ObjClosure> closure) : data(std::move(closure)) {}
    Value(std::shared_ptr<ObjArray> arr) : data(std::move(arr)) {}
    Value(std::shared_ptr<ObjDict> dict) : data(std::move(dict)) {}
    Value(std::shared_ptr<ObjClass> klass) : data(std::move(klass)) {}
    Value(std::shared_ptr<ObjInstance> inst) : data(std::move(inst)) {}
    Value(std::shared_ptr<ObjBoundMethod> bound) : data(std::move(bound)) {}
    Value(std::shared_ptr<ObjUserData> ud) : data(std::move(ud)) {}
    Value(std::shared_ptr<ObjFiber> fiber) : data(std::move(fiber)) {}
    Value(ObjVec2 v) : data(v) {}
    Value(ObjVec3 v) : data(v) {}
    Value(ObjColor c) : data(c) {}

    // Type checking
    bool is_nil() const { return std::holds_alternative<Nil>(data); }
    bool is_int() const { return std::holds_alternative<int64_t>(data); }
    bool is_double() const { return std::holds_alternative<double>(data); }
    bool is_bool() const { return std::holds_alternative<bool>(data); }
    bool is_string() const { return std::holds_alternative<std::string>(data); }
    bool is_number() const { return is_int() || is_double(); }
    bool is_native_fn() const { return std::holds_alternative<NativeFn>(data); }
    bool is_function() const { return std::holds_alternative<std::shared_ptr<ObjFunction>>(data); }
    bool is_closure() const { return std::holds_alternative<std::shared_ptr<ObjClosure>>(data); }
    bool is_array() const { return std::holds_alternative<std::shared_ptr<ObjArray>>(data); }
    bool is_dict() const { return std::holds_alternative<std::shared_ptr<ObjDict>>(data); }
    bool is_class() const { return std::holds_alternative<std::shared_ptr<ObjClass>>(data); }
    bool is_instance() const { return std::holds_alternative<std::shared_ptr<ObjInstance>>(data); }
    bool is_bound_method() const { return std::holds_alternative<std::shared_ptr<ObjBoundMethod>>(data); }
    bool is_user_data() const { return std::holds_alternative<std::shared_ptr<ObjUserData>>(data); }
    bool is_fiber() const { return std::holds_alternative<std::shared_ptr<ObjFiber>>(data); }
    bool is_vec2() const { return std::holds_alternative<ObjVec2>(data); }
    bool is_vec3() const { return std::holds_alternative<ObjVec3>(data); }
    bool is_color() const { return std::holds_alternative<ObjColor>(data); }

    // Extraction
    int64_t as_int() const { return std::get<int64_t>(data); }
    double as_double() const { return std::get<double>(data); }
    bool as_bool() const { return std::get<bool>(data); }
    const std::string& as_string() const { return std::get<std::string>(data); }
    NativeFn as_native_fn() const { return std::get<NativeFn>(data); }
    std::shared_ptr<ObjFunction> as_function() const { return std::get<std::shared_ptr<ObjFunction>>(data); }
    std::shared_ptr<ObjClosure> as_closure() const { return std::get<std::shared_ptr<ObjClosure>>(data); }
    std::shared_ptr<ObjArray> as_array() const { return std::get<std::shared_ptr<ObjArray>>(data); }
    std::shared_ptr<ObjDict> as_dict() const { return std::get<std::shared_ptr<ObjDict>>(data); }
    std::shared_ptr<ObjClass> as_class() const { return std::get<std::shared_ptr<ObjClass>>(data); }
    std::shared_ptr<ObjInstance> as_instance() const { return std::get<std::shared_ptr<ObjInstance>>(data); }
    std::shared_ptr<ObjBoundMethod> as_bound_method() const { return std::get<std::shared_ptr<ObjBoundMethod>>(data); }
    std::shared_ptr<ObjUserData> as_user_data() const { return std::get<std::shared_ptr<ObjUserData>>(data); }
    std::shared_ptr<ObjFiber> as_fiber() const { return std::get<std::shared_ptr<ObjFiber>>(data); }
    ObjVec2 as_vec2() const { return std::get<ObjVec2>(data); }
    ObjVec3 as_vec3() const { return std::get<ObjVec3>(data); }
    ObjColor as_color() const { return std::get<ObjColor>(data); }
    
    // Casting (e.g., getting a number as double regardless of int or double)
    double as_number() const
    {
        if (is_int()) return static_cast<double>(as_int());
        if (is_double()) return as_double();
        throw std::runtime_error("Value is not a number.");
    }

    // Equality
    bool operator==(const Value& other) const
    {
        // Simple variant equality handles everything!
        // But for numbers, we might want 1 == 1.0 to be true.
        if (is_number() && other.is_number())
        {
            return as_number() == other.as_number();
        }
        return data == other.data;
    }
    
    bool operator!=(const Value& other) const
    {
        return !(*this == other);
    }
};

struct ObjUpvalue
{
    u32 location = 0; // Absolute index in VM stack
    Value closed;
    bool is_closed = false;
};

struct ObjBoundMethod
{
    Value receiver;
    std::shared_ptr<ObjClosure> method;
};

std::string to_string(const Value& value);

} // namespace blades
