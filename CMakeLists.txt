cmake_minimum_required(VERSION 3.20)
project(AudioPlugins VERSION 0.0.7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Global options
option(BUILD_COMPRESSOR "Build Stinky Compressor plugin" ON)
option(BUILD_EQ "Build Stinky EQ plugin" ON)
option(BUILD_LIMITER "Build Stinky Limiter plugin" ON)
option(BUILD_DELAY "Build Stinky Delay plugin" ON)
option(BUILD_TESTS "Build test suite for all plugins" ON)
option(BUILD_BENCHMARKS "Build throughput benchmarks for all plugins" ON)
option(ENABLE_SIMD "Enable SIMD optimizations (compressor, limiter)" ON)

# Fetch CLAP SDK once for all plugins
include(FetchContent)
FetchContent_Declare(
    clap
    GIT_REPOSITORY https://github.com/free-audio/clap.git
    GIT_TAG 1.2.7
)
FetchContent_MakeAvailable(clap)

# Fetch Google Test once for all plugins (if testing is enabled)
if(BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    enable_testing()
endif()

# Add plugin subdirectories
if(BUILD_COMPRESSOR)
    add_subdirectory(compressor)
endif()

if(BUILD_EQ)
    add_subdirectory(eq)
endif()

if(BUILD_LIMITER)
    add_subdirectory(limiter)
endif()

if(BUILD_DELAY)
    add_subdirectory(delay)
endif()

# Aggregate benchmark runner. Numbers are only meaningful from a Release build:
#   cmake --build . --config Release --target bench
if(BUILD_BENCHMARKS)
    set(BENCH_TARGETS)
    foreach(target CompressorBench EqBench LimiterBench DelayBench)
        if(TARGET ${target})
            list(APPEND BENCH_TARGETS ${target})
        endif()
    endforeach()
    set(BENCH_COMMANDS)
    foreach(target ${BENCH_TARGETS})
        list(APPEND BENCH_COMMANDS COMMAND $<TARGET_FILE:${target}>)
    endforeach()
    add_custom_target(bench
        ${BENCH_COMMANDS}
        COMMENT "Running plugin throughput benchmarks"
        USES_TERMINAL
        VERBATIM
    )
    if(BENCH_TARGETS)
        add_dependencies(bench ${BENCH_TARGETS})
    endif()
endif()

# Emit third-party license notices alongside the build output so downstream
# packagers can copy complete attribution for what is compiled into the plugins.
# CLAP (MIT) is compiled into every shipped plugin; googletest/TypeScript are
# test-/build-only and are not redistributed, so they are not bundled here.
# Install into a project-namespaced subdirectory so that, when installed into a
# shared prefix that aggregates many libraries, these files don't collide with
# (or get mistaken for) the whole package's license.
set(THIRD_PARTY_LICENSE_DIR "${CMAKE_SOURCE_DIR}/licenses")
file(COPY "${THIRD_PARTY_LICENSE_DIR}/" DESTINATION "${CMAKE_BINARY_DIR}/licenses")
install(DIRECTORY "${THIRD_PARTY_LICENSE_DIR}/"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/licenses/audio-plugins"
)
install(FILES "${CMAKE_SOURCE_DIR}/LICENSE" "${CMAKE_SOURCE_DIR}/THIRD_PARTY_LICENSES.md"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/licenses/audio-plugins"
)

# Generate TypeScript definitions from C++ annotations
find_program(NODE_EXECUTABLE NAMES node nodejs)
if(NODE_EXECUTABLE)
    add_custom_target(generate_typescript
        COMMAND ${NODE_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/generate-from-annotations.js
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating TypeScript plugin definitions from C++ annotations"
        SOURCES
            ${CMAKE_SOURCE_DIR}/scripts/generate-from-annotations.js
            ${CMAKE_SOURCE_DIR}/compressor/include/compressor_clap.h
            ${CMAKE_SOURCE_DIR}/delay/include/delay_clap.h
            ${CMAKE_SOURCE_DIR}/eq/include/eq_clap.h
            ${CMAKE_SOURCE_DIR}/limiter/include/limiter_clap.h
    )
    message(STATUS "TypeScript generation target added (run: cmake --build . --target generate_typescript)")
else()
    message(WARNING "Node.js not found - TypeScript generation target will not be available")
endif()

# Print build summary
message(STATUS "")
message(STATUS "═══════════════════════════════════════")
message(STATUS "  Audio Plugins Build Configuration")
message(STATUS "═══════════════════════════════════════")
message(STATUS "  Compressor: ${BUILD_COMPRESSOR}")
message(STATUS "  EQ:         ${BUILD_EQ}")
message(STATUS "  Limiter:    ${BUILD_LIMITER}")
message(STATUS "  Tests:      ${BUILD_TESTS}")
message(STATUS "  Benchmarks: ${BUILD_BENCHMARKS}")
message(STATUS "  SIMD:       ${ENABLE_SIMD}")
message(STATUS "═══════════════════════════════════════")
message(STATUS "")
