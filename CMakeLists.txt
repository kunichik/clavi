cmake_minimum_required(VERSION 3.21)
project(clavi VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Options
option(CLAVI_BUILD_TESTS "Build unit tests" ON)
option(CLAVI_BUILD_DAEMON "Build clavid daemon" OFF)
option(CLAVI_ENABLE_LLM "Enable Layer 3 LLM detection" OFF)
option(CLAVI_BUILD_FUZZ "Build fuzz targets (requires clang -fsanitize=fuzzer)" OFF)

# Compiler flags
if(MSVC)
  add_compile_options(/W4 /utf-8 /permissive-)
  add_compile_options(/GR-) # no RTTI
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror -fno-rtti)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
  endif()
endif()

# Submodule deps
add_subdirectory(extern/tomlplusplus EXCLUDE_FROM_ALL)

# xxHash: header-only (XXH_INLINE_ALL)
add_library(xxhash_header INTERFACE)
target_include_directories(xxhash_header INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/extern/xxHash)
add_library(xxHash::xxhash ALIAS xxhash_header)

# Core library
add_subdirectory(core)

# Daemon (optional)
if(CLAVI_BUILD_DAEMON)
  add_subdirectory(daemon)
endif()

# Tests
if(CLAVI_BUILD_TESTS)
  enable_testing()
  add_subdirectory(extern/Catch2 EXCLUDE_FROM_ALL)
  add_subdirectory(tests)
endif()

# Fuzz targets (clang only)
if(CLAVI_BUILD_FUZZ)
  add_subdirectory(fuzz)
endif()
