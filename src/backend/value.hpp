#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// value.hpp — Runtime value representation for the VM
// ─────────────────────────────────────────────────────────────────────────────

#include <variant>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>

namespace blades
{

// We use std::monostate to represent 'nil'
using Nil = std::monostate;

struct Value;
using NativeFn = Value(*)(const std::vector<Value>& args);

struct Value
{
    std::variant<Nil, int, double, bool, std::string, NativeFn> data;

    // Constructors
    Value() : data(Nil{}) {}
    Value(Nil n) : data(n) {}
    Value(int i) : data(i) {}
    Value(double d) : data(d) {}
    Value(bool b) : data(b) {}
    Value(std::string s) : data(std::move(s)) {}
    Value(const char* s) : data(std::string(s)) {}
    Value(NativeFn fn) : data(fn) {}

    // Type checking
    bool is_nil() const { return std::holds_alternative<Nil>(data); }
    bool is_int() const { return std::holds_alternative<int>(data); }
    bool is_double() const { return std::holds_alternative<double>(data); }
    bool is_bool() const { return std::holds_alternative<bool>(data); }
    bool is_string() const { return std::holds_alternative<std::string>(data); }
    bool is_number() const { return is_int() || is_double(); }
    bool is_native_fn() const { return std::holds_alternative<NativeFn>(data); }

    // Extraction
    int as_int() const { return std::get<int>(data); }
    double as_double() const { return std::get<double>(data); }
    bool as_bool() const { return std::get<bool>(data); }
    const std::string& as_string() const { return std::get<std::string>(data); }
    NativeFn as_native_fn() const { return std::get<NativeFn>(data); }
    
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

std::string to_string(const Value& value);

} // namespace blades
