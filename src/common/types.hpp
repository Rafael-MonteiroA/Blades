#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// types.hpp — Fundamental type aliases for the Blades compiler
//
// All code in the Blades compiler should use these aliases instead of raw
// C++ primitive types to ensure portability and readability.
// ─────────────────────────────────────────────────────────────────────────────

#include <cstddef>
#include <cstdint>

namespace blades
{

// ── Unsigned integers ─────────────────────────────────────────────────────────
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// ── Signed integers ───────────────────────────────────────────────────────────
using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

// ── Floating point ────────────────────────────────────────────────────────────
using f32 = float;
using f64 = double;

// ── Platform-sized types ──────────────────────────────────────────────────────
using usize = std::size_t;
using isize = std::ptrdiff_t;

// ── Raw byte ──────────────────────────────────────────────────────────────────
using byte = u8;

} // namespace blades
