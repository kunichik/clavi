add_library(clavi-core STATIC
    src/layout_map.cpp
    src/dictionary.cpp
    src/detector.cpp
    src/pack_loader.cpp
    src/config.cpp
    src/undo_stack.cpp
    src/translit.cpp
)

target_include_directories(clavi-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(clavi-core
    PUBLIC
        tomlplusplus::tomlplusplus
    PRIVATE
        xxHash::xxhash
)

set_target_properties(clavi-core PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
