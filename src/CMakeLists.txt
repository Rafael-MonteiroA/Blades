# ── blades_lib: core compiler/VM library ─────────────────────────────────────
add_library(blades_lib STATIC
    common/source_location.cpp
    compiler/token.cpp
    compiler/lexer.cpp
    compiler/ast.cpp
    compiler/parser.cpp
    compiler/value_type.cpp
    compiler/semantic_analyzer.cpp
    compiler/ir.cpp
    compiler/ir_generator.cpp
    backend/value.cpp
    backend/vm.cpp
    runtime/stdlib.cpp
)

target_include_directories(blades_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated   # for version.hpp
)

target_link_libraries(blades_lib
    PUBLIC blades_warnings
)

if(BLADES_ENABLE_RAYLIB)
    target_compile_definitions(blades_lib PUBLIC BLADES_HAS_RAYLIB=1)
    target_link_libraries(blades_lib PUBLIC raylib)
endif()

# ── blades: main executable ───────────────────────────────────────────────────
add_executable(blades
    main.cpp
)

target_link_libraries(blades
    PRIVATE blades_lib
)

if(EXISTS "${CMAKE_SOURCE_DIR}/test_phase1.bl")
    add_test(NAME blades_phase1 COMMAND blades "${CMAKE_SOURCE_DIR}/test_phase1.bl")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/test_match.bl")
    add_test(NAME blades_match COMMAND blades "${CMAKE_SOURCE_DIR}/test_match.bl")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/test_geral.bl")
    add_test(NAME blades_general COMMAND blades "${CMAKE_SOURCE_DIR}/test_geral.bl")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/typed_features.bl")
    add_test(NAME blades_typed_features COMMAND blades "${CMAKE_SOURCE_DIR}/tests/typed_features.bl")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/type_errors.bl")
    add_test(NAME blades_type_errors COMMAND blades "${CMAKE_SOURCE_DIR}/tests/type_errors.bl")
    set_tests_properties(blades_type_errors PROPERTIES WILL_FAIL TRUE)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/variable_type_error.bl")
    add_test(NAME blades_variable_type_error COMMAND blades "${CMAKE_SOURCE_DIR}/tests/variable_type_error.bl")
    set_tests_properties(blades_variable_type_error PROPERTIES WILL_FAIL TRUE)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/syntax_error.bl")
    add_test(NAME blades_syntax_error COMMAND blades "${CMAKE_SOURCE_DIR}/tests/syntax_error.bl")
    set_tests_properties(blades_syntax_error PROPERTIES WILL_FAIL TRUE)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/import_main.bl")
    add_test(NAME blades_relative_import COMMAND blades "${CMAKE_SOURCE_DIR}/tests/import_main.bl")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/native_error.bl")
    add_test(NAME blades_native_error COMMAND blades "${CMAKE_SOURCE_DIR}/tests/native_error.bl")
    set_tests_properties(blades_native_error PROPERTIES WILL_FAIL TRUE)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/closure_types.bl")
    add_test(NAME blades_closure_types COMMAND blades "${CMAKE_SOURCE_DIR}/tests/closure_types.bl")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/cli_check.bl")
    add_test(NAME blades_cli_check COMMAND blades --check "${CMAKE_SOURCE_DIR}/tests/cli_check.bl")
    add_test(NAME blades_cli_disassemble COMMAND blades --disassemble "${CMAKE_SOURCE_DIR}/tests/cli_check.bl")
endif()
if(BLADES_ENABLE_RAYLIB AND EXISTS "${CMAKE_SOURCE_DIR}/examples/fps_table.bl")
    add_test(NAME blades_fps_table_check COMMAND blades --check "${CMAKE_SOURCE_DIR}/examples/fps_table.bl")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/type_errors.bl")
    add_test(NAME blades_cli_check_error COMMAND blades --check "${CMAKE_SOURCE_DIR}/tests/type_errors.bl")
    set_tests_properties(blades_cli_check_error PROPERTIES WILL_FAIL TRUE)
endif()
