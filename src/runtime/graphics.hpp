#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// graphics.hpp - Graphics and Windowing bindings for Blades VM (using Raylib)
// ─────────────────────────────────────────────────────────────────────────────

namespace blades
{
class VM;
void register_graphics_functions(VM& vm);
}
