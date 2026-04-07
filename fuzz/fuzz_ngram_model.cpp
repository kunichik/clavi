// Fuzz target for NgramModel::load + score — feeds arbitrary binary blobs
// as ngram.bin data, then scores random text through it.

#include "clavi/ngram_model.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > 65536) return 0;

    const char* tmp_path = "fuzz_ngram_tmp.bin";
    {
        std::FILE* f = std::fopen(tmp_path, "wb");
        if (!f) return 0;
        std::fwrite(data, 1, size, f);
        std::fclose(f);
    }

    clavi::NgramModel model;
    if (model.load(tmp_path)) {
        (void)model.score("hello world");
        (void)model.score("\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82");
        (void)model.score("");

        // Score the raw fuzz data as text
        const std::string_view as_text(reinterpret_cast<const char*>(data), size);
        (void)model.score(as_text);
    }

    std::remove(tmp_path);
    return 0;
}
