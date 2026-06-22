# ---------------------------------------------------------------------------
# Test suite: cachesim_tests
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://codeload.github.com/google/googletest/tar.gz/refs/tags/v1.15.2
)

# Prevent GoogleTest from overriding our parent project's compiler/linker
# settings (it tries to force its own runtime library selection on MSVC,
# irrelevant here but good practice to set).
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(cachesim_tests
    test_CacheLine.cpp
    test_AddressDecomposition.cpp
    test_LRUPolicy.cpp
    test_FIFOPolicy.cpp
    test_RandomPolicy.cpp
    test_CacheSet.cpp
    test_Cache_DirectMapped.cpp
    test_Cache_SetAssociative.cpp
    test_Cache_FullyAssociative.cpp
    test_WritePolicies.cpp
    test_MemoryHierarchy.cpp
    test_TraceParser.cpp
)

target_link_libraries(cachesim_tests
    PRIVATE
        cachesim
        GTest::gtest_main
)

target_compile_options(cachesim_tests PRIVATE ${CACHESIM_WARNING_FLAGS})

# Registers each TEST()/TEST_F() case with ctest individually, so
# `ctest --output-on-failure` gives per-test granularity rather than
# one pass/fail blob for the whole binary.
include(GoogleTest)
gtest_discover_tests(cachesim_tests)
