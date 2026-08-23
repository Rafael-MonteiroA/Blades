#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// stdlib.hpp — Standard library functions for Blades
// ─────────────────────────────────────────────────────────────────────────────

#include "backend/vm.hpp"

namespace blades
{

void register_stdlib(VM& vm);

} // namespace blades
