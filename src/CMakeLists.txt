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
    PUBLIC blades_warnings raylib
)

# ── blades: main executable ───────────────────────────────────────────────────
add_executable(blades
    main.cpp
)

target_link_libraries(blades
    PRIVATE blades_lib
)
