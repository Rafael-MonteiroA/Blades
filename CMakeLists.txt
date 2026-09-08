cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_policy(SET CMP0135 NEW)

project(
    blades
    VERSION 0.1.0
    DESCRIPTION "The Blades Programming Language"
    LANGUAGES CXX
)

enable_testing()

option(BLADES_ENABLE_RAYLIB "Build the optional Raylib graphics module" ON)

# ── C++ Standard ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Output directories ────────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ── Compiler warnings ─────────────────────────────────────────────────────────
add_library(blades_warnings INTERFACE)

if(MSVC)
    target_compile_options(blades_warnings INTERFACE
        /W4
        /WX             # Warnings as errors
        /permissive-    # Strict conformance mode
        /Zc:__cplusplus # Correct __cplusplus macro
    )
else()
    target_compile_options(blades_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wformat=2
    )
endif()

# ── Version header ────────────────────────────────────────────────────────────
configure_file(
    ${CMAKE_SOURCE_DIR}/src/common/version.hpp.in
    ${CMAKE_BINARY_DIR}/generated/version.hpp
    @ONLY
)

# ── Subdirectories ────────────────────────────────────────────────────────────

if(BLADES_ENABLE_RAYLIB)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
    )
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

add_subdirectory(src)
