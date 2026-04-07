// Fuzz target for Detector::analyze — feeds arbitrary UTF-8 blobs.
// Build: clang++ -fsanitize=fuzzer,address -O1 -g -std=c++20 ...
// See fuzz/CMakeLists.txt for the build integration.

#include "clavi/detector.hpp"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string_view>

static clavi::Detector& get_detector() {
    static clavi::Detector det = []() {
        clavi::Detector d;
        namespace fs = std::filesystem;
        // Try common pack locations (CI + local dev)
        for (const auto& base : {"packs", "../packs", "../../packs"}) {
            if (fs::is_directory(fs::path(base) / "uk") &&
                fs::is_directory(fs::path(base) / "en")) {
                (void)d.load_pack((fs::path(base) / "uk").string());
                (void)d.load_pack((fs::path(base) / "en").string());
                break;
            }
        }
        return d;
    }();
    return det;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Cap input to avoid OOM on huge inputs
    if (size > 4096) return 0;

    const std::string_view input(reinterpret_cast<const char*>(data), size);
    auto& det = get_detector();

    // Exercise both analyze and score_ngrams paths
    (void)det.analyze(input);

    return 0;
}
