# StinkyDelay Plugin
project(StinkyDelay VERSION 1.0.0 LANGUAGES CXX)

# Use parent's C++ standard if not set
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
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
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Source files
set(SOURCES
    src/delay_processor.cc
    src/delay_clap.cc
)

set(HEADERS
    include/delay_processor.h
    include/delay_clap.h
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
    add_executable(DelayTests
        tests/test_clap_plugin.cc
    )
    
    target_include_directories(DelayTests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${clap_SOURCE_DIR}/include
    )
    
    target_link_libraries(DelayTests
        PRIVATE
            gtest_main
    )
    
    # Add sources directly to test
    target_sources(DelayTests PRIVATE
        src/delay_processor.cc
        src/delay_clap.cc
    )
    
    include(GoogleTest)
    gtest_discover_tests(DelayTests)
endif()

# Install plugin and TypeScript definitions
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/plugins
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/plugins
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/delay-plugin.ts
    DESTINATION ${CMAKE_INSTALL_PREFIX}/plugins
)

# Print configuration
message(STATUS "")
message(STATUS "═══════════════════════════════════════")
message(STATUS "  ${PROJECT_NAME} Configuration")
message(STATUS "═══════════════════════════════════════")
message(STATUS "  Version:    ${PROJECT_VERSION}")
message(STATUS "  Tests:      ${BUILD_TESTS}")
message(STATUS "═══════════════════════════════════════")
message(STATUS "")
