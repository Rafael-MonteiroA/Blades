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

    void declare(std::string_view name, ValueType type)
    {
        if (m_scopes.empty())
        {
            m_scopes.push_back({});
        }
        m_scopes.back()[std::string(name)] = type;
    }

    std::optional<ValueType> lookup(std::string_view name) const
    {
        for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it)
        {
            auto found = it->find(std::string(name));
            if (found != it->end())
            {
                return found->second;
            }
        }
        return std::nullopt;
    }

private:
    std::vector<std::unordered_map<std::string, ValueType>> m_scopes;
};

} // namespace blades
