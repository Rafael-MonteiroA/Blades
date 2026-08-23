# ── blades_tests: test executable ────────────────────────────────────────────
add_executable(blades_tests
    test_main.cpp
)

target_include_directories(blades_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated   # for version.hpp
)

target_link_libraries(blades_tests
    PRIVATE blades_lib
)

# Register with CTest
enable_testing()
add_test(NAME blades_tests COMMAND blades_tests)
