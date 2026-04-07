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

# ── CPack packaging ──────────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME "clavi")
set(CPACK_PACKAGE_VENDOR "Clavi Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Automatic keyboard layout switcher")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# DEB
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Clavi Project")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libx11-6, libxtst6")

# RPM
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
set(CPACK_RPM_PACKAGE_GROUP "Applications/System")

# Install packs alongside daemon
install(DIRECTORY packs/ DESTINATION share/clavi/packs
        FILES_MATCHING PATTERN "*.bin" PATTERN "*.toml")
install(DIRECTORY docs/ DESTINATION share/doc/clavi
        FILES_MATCHING PATTERN "*.toml" PATTERN "*.md")
install(FILES deploy/linux/clavi.service
        DESTINATION lib/systemd/user OPTIONAL)

include(CPack)
