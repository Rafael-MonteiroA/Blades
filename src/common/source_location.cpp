// ─────────────────────────────────────────────────────────────────────────────
// source_location.cpp — Implementation of SourceLocation utilities
// ─────────────────────────────────────────────────────────────────────────────

#include "common/source_location.hpp"

#include <sstream>

namespace blades
{

std::string SourceLocation::to_string() const
{
    std::ostringstream oss;
    if (!filename.empty())
        oss << filename << ':';
    oss << line << ':' << column;
    return oss.str();
}

} // namespace blades
