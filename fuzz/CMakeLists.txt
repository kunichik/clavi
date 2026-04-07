# Fuzz targets — requires Clang with -fsanitize=fuzzer support.
# Build: cmake -B build-fuzz -G Ninja -DCMAKE_CXX_COMPILER=clang++
#        -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCLAVI_BUILD_FUZZ=ON
#        cmake --build build-fuzz

foreach(target fuzz_detector fuzz_layout_map fuzz_ngram_model)
    add_executable(${target} ${target}.cpp)

    target_include_directories(${target}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/core/include
    )

    target_link_libraries(${target}
        PRIVATE clavi-core
    )

    target_compile_options(${target}
        PRIVATE -fsanitize=fuzzer,address -fno-omit-frame-pointer
    )

    target_link_options(${target}
        PRIVATE -fsanitize=fuzzer,address
    )

    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )
endforeach()
