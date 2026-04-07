add_executable(clavi-tests
    unit/test_layout_map.cpp
    unit/test_dictionary.cpp
    unit/test_detector.cpp
    unit/test_pack_loader.cpp
    unit/test_config.cpp
    unit/test_undo_stack.cpp
    unit/test_translit.cpp
    unit/test_word_buffer.cpp
    unit/test_ngram_model.cpp
    unit/test_logger.cpp
    integration/test_detector_e2e.cpp
    # Compile word_buffer directly (no libuiohook dependency)
    ${CMAKE_SOURCE_DIR}/daemon/src/word_buffer.cpp
)

target_link_libraries(clavi-tests
    PRIVATE
        clavi-core
        Catch2::Catch2WithMain
        xxHash::xxhash
)

target_include_directories(clavi-tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/daemon/include
)

set_target_properties(clavi-tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

include(CTest)
include(Catch)
catch_discover_tests(clavi-tests)
