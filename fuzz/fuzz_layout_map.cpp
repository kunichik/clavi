// Fuzz target for LayoutMap::load + remap — feeds arbitrary binary blobs
// as keyboard_map.bin data, then remaps random UTF-8 text through it.
// Build: clang++ -fsanitize=fuzzer,address -O1 -g -std=c++20 ...

#include "clavi/layout_map.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > 65536) return 0;

    // Write raw fuzz data as a temporary keyboard_map.bin
    const char* tmp_path = "fuzz_kmap_tmp.bin";
    {
        std::FILE* f = std::fopen(tmp_path, "wb");
        if (!f) return 0;
        std::fwrite(data, 1, size, f);
        std::fclose(f);
    }

    clavi::LayoutMap lmap;
    if (lmap.load(tmp_path)) {
        // Exercise remap with a fixed test string (Cyrillic + Latin mix)
        (void)lmap.remap("hello \xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82");
        (void)lmap.remap("abcdefghijklmnopqrstuvwxyz");
        (void)lmap.remap("");

        // Also try remapping the raw fuzz input itself as text
        const std::string_view as_text(reinterpret_cast<const char*>(data), size);
        (void)lmap.remap(as_text);
    }

    std::remove(tmp_path);
    return 0;
}
