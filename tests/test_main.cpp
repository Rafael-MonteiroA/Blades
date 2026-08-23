// ─────────────────────────────────────────────────────────────────────────────
// test_main.cpp — Entry point for the Blades test suite
//
// All test files are linked into this executable. Tests self-register via
// static initialization (see framework/test.hpp), so this file only needs
// to call run_all().
// ─────────────────────────────────────────────────────────────────────────────

#include "framework/test.hpp"

// ── Include test modules ──────────────────────────────────────────────────────
#include "test_result.hpp"
#include "test_source_location.hpp"
#include "test_lexer.hpp"
#include "test_parser.hpp"
#include "test_semantic.hpp"
#include "test_ir.hpp"
#include "test_vm.hpp"
#include "test_runtime.hpp"

int main()
{
    return blades::test::run_all();
}
