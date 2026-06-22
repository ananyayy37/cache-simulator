cmake_minimum_required(VERSION 3.16)

# Use NEW timestamp behavior for FetchContent-downloaded archives (silences
# a harmless but noisy CMake dev-warning on 3.24+).
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(CacheSimulator
    VERSION 0.1.0
    DESCRIPTION "Cache and Memory Hierarchy Simulator"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Global project settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)        # disable compiler-specific extensions (e.g. GNU++17) for portability

# Default to a Release build if the user didn't specify one.
# Debug-by-default is a common footgun: people forget -O2 exists and then
# wonder why their "fast" simulator takes 40 seconds on a 1M-line trace.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Always export compile_commands.json — used by clangd / IDE tooling.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Warnings-as-near-errors. We don't use -Werror project-wide because
# FetchContent'd third-party headers (GoogleTest, nlohmann/json) may emit
# warnings we don't control. Our own targets opt into strict warnings below.
set(CACHESIM_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
)

# ---------------------------------------------------------------------------
# Dependencies (fetched at configure time, no manual installation required)
# ---------------------------------------------------------------------------
include(FetchContent)

# nlohmann/json — header-only JSON library, used for cache config files.
FetchContent_Declare(
    json
    URL https://codeload.github.com/nlohmann/json/tar.gz/refs/tags/v3.11.3
)
FetchContent_MakeAvailable(json)

# GoogleTest — only needed for the test target, fetched conditionally below.
option(CACHESIM_BUILD_TESTS "Build unit tests" ON)

# ---------------------------------------------------------------------------
# Core library: cachesim
# All simulator logic lives here. Both the CLI and the test suite link
# against this target so there is exactly one compiled copy of the logic
# under test — no drift between "what we test" and "what we ship".
# ---------------------------------------------------------------------------
add_library(cachesim STATIC
    src/CacheLine.cpp
    src/LRUPolicy.cpp
    src/FIFOPolicy.cpp
    src/RandomPolicy.cpp
    src/CacheSet.cpp
    src/CacheConfig.cpp
    src/CacheStats.cpp
    src/Cache.cpp
    src/MemoryHierarchy.cpp
    src/TraceParser.cpp
)

target_include_directories(cachesim
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(cachesim
    PUBLIC
        nlohmann_json::nlohmann_json
)

target_compile_options(cachesim PRIVATE ${CACHESIM_WARNING_FLAGS})

# ---------------------------------------------------------------------------
# CLI executable: cachesim_cli
# Thin entry point — parses args, loads a config + trace, runs the
# simulation, prints/exports stats. All real logic stays in the library.
# ---------------------------------------------------------------------------
add_executable(cachesim_cli src/main.cpp)
target_link_libraries(cachesim_cli PRIVATE cachesim)
target_compile_options(cachesim_cli PRIVATE ${CACHESIM_WARNING_FLAGS})

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(CACHESIM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
