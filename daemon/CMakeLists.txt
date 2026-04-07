cmake_minimum_required(VERSION 3.21)

# ── libuiohook ──────────────────────────────────────────────────────────────
set(UIOHOOK_BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(UIOHOOK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/extern/libuiohook
                 ${CMAKE_BINARY_DIR}/extern/libuiohook
                 EXCLUDE_FROM_ALL)

# ── clavid executable ───────────────────────────────────────────────────────
set(DAEMON_SOURCES
    src/main.cpp
    src/hook.cpp
    src/word_buffer.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND DAEMON_SOURCES
        src/platform/switcher_linux.cpp
        src/platform/toast_linux.cpp
        src/platform/tray_linux.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND DAEMON_SOURCES
        src/platform/switcher_mac.cpp
        src/platform/toast_mac.cpp
        src/platform/tray_mac.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND DAEMON_SOURCES
        src/platform/switcher_win.cpp
        src/platform/toast_win.cpp
        src/platform/tray_win.cpp
    )
endif()

add_executable(clavid ${DAEMON_SOURCES})

target_include_directories(clavid
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/core/include
        ${CMAKE_SOURCE_DIR}/extern/libuiohook/include
)

target_link_libraries(clavid
    PRIVATE
        clavi-core
        uiohook
)

set_target_properties(clavid PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(clavid PRIVATE
        "-framework Carbon"
        "-framework ApplicationServices"
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(clavid PRIVATE imm32 ole32 oleaut32)
endif()

install(TARGETS clavid RUNTIME DESTINATION bin)
