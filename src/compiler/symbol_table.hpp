#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// symbol_table.hpp — Tracks variables and their types across scopes
// ─────────────────────────────────────────────────────────────────────────────

#include <string_view>
#include <unordered_map>
#include <vector>
#include <optional>
#include <string>

#include "compiler/value_type.hpp"

namespace blades
{

class SymbolTable
{
public:
    struct Symbol
    {
        ValueType type = ValueType::Unknown;
        bool mutable_binding = true;
    };

    void begin_scope()
    {
        m_scopes.push_back({});
    }

    void end_scope()
    {
        if (!m_scopes.empty())
        {
            m_scopes.pop_back();
        }
    }

    void declare(std::string_view name, ValueType type, bool mutable_binding = true)
    {
        if (m_scopes.empty())
        {
            m_scopes.push_back({});
        }
        m_scopes.back()[std::string(name)] = Symbol{type, mutable_binding};
    }

    std::optional<ValueType> lookup(std::string_view name) const
    {
        for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it)
        {
            auto found = it->find(std::string(name));
            if (found != it->end())
            {
                return found->second.type;
            }
        }
        return std::nullopt;
    }

    std::optional<Symbol> lookup_symbol(std::string_view name) const
    {
        for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it)
        {
            auto found = it->find(std::string(name));
            if (found != it->end()) return found->second;
        }
        return std::nullopt;
    }

    bool is_mutable(std::string_view name) const
    {
        auto symbol = lookup_symbol(name);
        return symbol.has_value() && symbol->mutable_binding;
    }

private:
    std::vector<std::unordered_map<std::string, Symbol>> m_scopes;
};

} // namespace blades
