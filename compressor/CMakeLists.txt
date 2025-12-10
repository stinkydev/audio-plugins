# StinkyCompressor Plugin
project(StinkyCompressor VERSION 1.0.0 LANGUAGES CXX)

# Use parent's C++ standard if not set
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# Build options (use parent option if available)
if(NOT DEFINED ENABLE_SIMD)
    option(ENABLE_SIMD "Enable SIMD optimizations" ON)
endif()

# Fetch CLAP SDK only if not already available
if(NOT TARGET clap)
    include(FetchContent)
    FetchContent_Declare(
        clap
        GIT_REPOSITORY https://github.com/free-audio/clap.git
        GIT_TAG 1.2.7
    )
    FetchContent_MakeAvailable(clap)
endif()

# Platform-specific settings
if(MSVC)
    add_compile_options(/W4 /WX)
    if(ENABLE_SIMD)
        add_compile_options(/arch:AVX2)
        add_compile_definitions(USE_SIMD=1)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    if(ENABLE_SIMD)
        add_compile_options(-mavx2 -mfma)
        add_compile_definitions(USE_SIMD=1)
    endif()
endif()

# Source files
set(SOURCES
    src/compressor_processor.cc
    src/compressor_clap.cc
    src/simd_utils.cc
)

set(HEADERS
    include/compressor_processor.h
    include/compressor_clap.h
    include/simd_utils.h
)

# Create the CLAP plugin as a shared library
add_library(${PROJECT_NAME} MODULE ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${clap_SOURCE_DIR}/include
)

# Platform-specific library settings for CLAP
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX ".clap"
        PREFIX ""
    )
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "clap"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist"
    )
elseif(UNIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX ".clap"
        PREFIX ""
    )
endif()

# Set visibility for CLAP entry point
if(APPLE OR UNIX)
    target_compile_options(${PROJECT_NAME} PRIVATE -fvisibility=hidden)
endif()

# Testing
if(NOT DEFINED BUILD_TESTS)
    option(BUILD_TESTS "Build test suite" ON)
endif()

if(BUILD_TESTS)
    if(NOT TARGET gtest_main)
        enable_testing()
        
        # Fetch Google Test
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
        )
        FetchContent_MakeAvailable(googletest)
    endif()
    
    # Test sources
    set(TEST_SOURCES
        tests/test_compressor_processor.cc
        tests/test_simd_utils.cc
        tests/test_clap_plugin.cc
    )
    
    # Create test executable
    add_executable(CompressorTests ${TEST_SOURCES})
    
    target_link_libraries(CompressorTests
        PRIVATE
            gtest_main
            gmock
    )
    
    target_include_directories(CompressorTests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${clap_SOURCE_DIR}/include
    )
    
    # Link test with source files (not the plugin library)
    target_sources(CompressorTests
        PRIVATE
            src/compressor_processor.cc
            src/compressor_clap.cc
            src/simd_utils.cc
    )
    
    target_compile_definitions(CompressorTests
        PRIVATE
            ${PROJECT_NAME}_EXPORTS
    )
    
    if(MSVC)
        target_compile_options(CompressorTests PRIVATE /W4)
    else()
        target_compile_options(CompressorTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    
    # Register tests with CTest
    include(GoogleTest)
    gtest_discover_tests(CompressorTests)
endif()
