cmake_minimum_required(VERSION 3.20)
project(AudioPlugins VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Global options
option(BUILD_COMPRESSOR "Build Sesame Compressor plugin" ON)
option(BUILD_EQ "Build Sesame EQ plugin" ON)
option(BUILD_TESTS "Build test suite for all plugins" ON)
option(ENABLE_SIMD "Enable SIMD optimizations (compressor)" ON)

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

# Print build summary
message(STATUS "")
message(STATUS "═══════════════════════════════════════")
message(STATUS "  Audio Plugins Build Configuration")
message(STATUS "═══════════════════════════════════════")
message(STATUS "  Compressor: ${BUILD_COMPRESSOR}")
message(STATUS "  EQ:         ${BUILD_EQ}")
message(STATUS "  Tests:      ${BUILD_TESTS}")
message(STATUS "  SIMD:       ${ENABLE_SIMD}")
message(STATUS "═══════════════════════════════════════")
message(STATUS "")
