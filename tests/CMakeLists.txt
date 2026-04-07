add_executable(clavi-tests
    unit/test_layout_map.cpp
    unit/test_dictionary.cpp
    unit/test_detector.cpp
    unit/test_pack_loader.cpp
    unit/test_config.cpp
    unit/test_undo_stack.cpp
    unit/test_translit.cpp
)

target_link_libraries(clavi-tests
    PRIVATE
        clavi-core
        Catch2::Catch2WithMain
)

set_target_properties(clavi-tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

include(CTest)
include(Catch)
catch_discover_tests(clavi-tests)
