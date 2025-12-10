# SesameLimiter Plugin
project(SesameLimiter VERSION 1.0.0 LANGUAGES CXX)

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
    src/limiter_processor.cc
    src/limiter_clap.cc
    src/simd_utils.cc
)

set(HEADERS
    include/limiter_processor.h
    include/limiter_clap.h
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
    
    # Test executable
    add_executable(LimiterTests
        tests/test_clap_plugin.cc
    )
    
    target_include_directories(LimiterTests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${clap_SOURCE_DIR}/include
    )
    
    target_link_libraries(LimiterTests
        PRIVATE
            gtest_main
    )
    
    # Add sources directly to test (for white-box testing)
    target_sources(LimiterTests PRIVATE
        src/limiter_processor.cc
        src/limiter_clap.cc
        src/simd_utils.cc
    )
    
    # Copy test resources
    add_custom_command(TARGET LimiterTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${PROJECT_NAME}>
        $<TARGET_FILE_DIR:LimiterTests>
    )
    
    include(GoogleTest)
    gtest_discover_tests(LimiterTests)
endif()

# Installation
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/clap"
    BUNDLE DESTINATION "${CMAKE_INSTALL_PREFIX}"
)

# Print configuration
message(STATUS "")
message(STATUS "═══════════════════════════════════════")
message(STATUS "  ${PROJECT_NAME} Configuration")
message(STATUS "═══════════════════════════════════════")
message(STATUS "  Version:    ${PROJECT_VERSION}")
message(STATUS "  SIMD:       ${ENABLE_SIMD}")
message(STATUS "  Tests:      ${BUILD_TESTS}")
message(STATUS "═══════════════════════════════════════")
message(STATUS "")
