#include <catch2/catch_test_macros.hpp>
#include "clavi/layout_map.hpp"

#include <filesystem>
#include <fstream>
#include <cstring>

namespace {

// Build a minimal keyboard_map.bin in memory and write to a temp file
// Format: KMAP + u32 count + N * (u32 source + u32 target), sorted by source
std::string write_temp_kmap(const std::vector<std::pair<uint32_t, uint32_t>>& pairs,
                             const std::string& suffix = "") {
    const std::string fname = "/test_kmap" + suffix + ".bin";
    std::string path = std::filesystem::temp_directory_path().string() + fname;
    std::ofstream f(path, std::ios::binary);
    const uint8_t magic[4] = {'K', 'M', 'A', 'P'};
    f.write(reinterpret_cast<const char*>(magic), 4);
    const uint32_t count = static_cast<uint32_t>(pairs.size());
    f.write(reinterpret_cast<const char*>(&count), 4);
    for (const auto& [src, tgt] : pairs) {
        f.write(reinterpret_cast<const char*>(&src), 4);
        f.write(reinterpret_cast<const char*>(&tgt), 4);
    }
    return path;
}

} // namespace

TEST_CASE("LayoutMap: loads empty file gracefully", "[layout_map]") {
    clavi::LayoutMap lm;
    REQUIRE_FALSE(lm.load("/nonexistent/path/kmap.bin"));
    REQUIRE(lm.empty());
}

TEST_CASE("LayoutMap: loads valid binary", "[layout_map]") {
    // Map: 'g' (0x67) -> 'п' (U+043F), 'h' -> 'р' (U+0440)
    std::vector<std::pair<uint32_t, uint32_t>> pairs = {
        {0x67, 0x043F},  // g -> п
        {0x68, 0x0440},  // h -> р
    };
    // Sort by source
    std::sort(pairs.begin(), pairs.end());
    const auto path = write_temp_kmap(pairs);

    clavi::LayoutMap lm;
    REQUIRE(lm.load(path));
    REQUIRE(lm.size() == 2);
}

TEST_CASE("LayoutMap: remap_codepoint returns mapped value", "[layout_map]") {
    std::vector<std::pair<uint32_t, uint32_t>> pairs = {
        {0x67, 0x043F},  // g -> п
        {0x68, 0x0440},  // h -> р
    };
    std::sort(pairs.begin(), pairs.end());
    const auto path = write_temp_kmap(pairs);

    clavi::LayoutMap lm;
    REQUIRE(lm.load(path));

    const auto result = lm.remap_codepoint(0x67);
    REQUIRE(result.has_value());
    REQUIRE(*result == 0x043F);
}

TEST_CASE("LayoutMap: remap_codepoint returns nullopt for unmapped", "[layout_map]") {
    std::vector<std::pair<uint32_t, uint32_t>> pairs = {{0x67, 0x043F}};
    const auto path = write_temp_kmap(pairs);

    clavi::LayoutMap lm;
    REQUIRE(lm.load(path));

    const auto result = lm.remap_codepoint(0x41); // 'A' — not in map
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("LayoutMap: remap passes through unmapped characters", "[layout_map]") {
    std::vector<std::pair<uint32_t, uint32_t>> pairs = {{0x67, 0x043F}};
    const auto path = write_temp_kmap(pairs);

    clavi::LayoutMap lm;
    REQUIRE(lm.load(path));

    // 'x' (0x78) is not in the map — should pass through unchanged
    const std::string result = lm.remap("x");
    REQUIRE(result == "x");
}

TEST_CASE("LayoutMap: EN to UK roundtrip", "[layout_map]") {
    // Simulate a symmetric pair: g->п and п->g
    std::vector<std::pair<uint32_t, uint32_t>> en_to_uk = {
        {0x67, 0x043F},  // g -> п
    };
    std::vector<std::pair<uint32_t, uint32_t>> uk_to_en = {
        {0x043F, 0x67},  // п -> g
    };
    const auto path_fwd = write_temp_kmap(en_to_uk, "_fwd");
    const auto path_rev = write_temp_kmap(uk_to_en, "_rev");

    clavi::LayoutMap en2uk, uk2en;
    REQUIRE(en2uk.load(path_fwd));
    REQUIRE(uk2en.load(path_rev));

    const std::string remapped = en2uk.remap("g");
    REQUIRE(remapped == "\xD0\xBF"); // UTF-8 for п (U+043F)

    const std::string roundtrip = uk2en.remap(remapped);
    REQUIRE(roundtrip == "g");
}

TEST_CASE("LayoutMap: rejects bad magic", "[layout_map]") {
    std::string path = std::filesystem::temp_directory_path().string() + "/bad_magic.bin";
    std::ofstream f(path, std::ios::binary);
    const uint8_t bad[8] = {'B', 'A', 'D', '!', 0, 0, 0, 0};
    f.write(reinterpret_cast<const char*>(bad), 8);
    f.close();

    clavi::LayoutMap lm;
    REQUIRE_FALSE(lm.load(path));
}
