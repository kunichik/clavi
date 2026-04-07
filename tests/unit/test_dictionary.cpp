#include <catch2/catch_test_macros.hpp>
#include "clavi/dictionary.hpp"

#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>

#define XXH_INLINE_ALL
#include <xxhash.h>

namespace {

// Build a dictionary.bin with the given words
std::string write_temp_dict(const std::vector<std::string>& words) {
    std::string path = std::filesystem::temp_directory_path().string() + "/test_dict.bin";

    // Compute load factor table size: ceil(count / 0.7), next power of 2
    const std::size_t count = words.size();
    std::size_t table_size = static_cast<std::size_t>(std::ceil(static_cast<double>(count) / 0.7));
    // Next power of 2
    std::size_t p2 = 1;
    while (p2 < table_size) p2 <<= 1;
    table_size = p2;

    std::vector<uint64_t> table(table_size, 0ULL);

    for (const auto& word : words) {
        // Lowercase
        std::string lower;
        for (unsigned char c : word) lower += static_cast<char>(std::tolower(c));
        uint64_t h = XXH3_64bits(lower.data(), lower.size());
        if (h == 0) h = 1;
        std::size_t idx = static_cast<std::size_t>(h) & (table_size - 1);
        while (table[idx] != 0) idx = (idx + 1) & (table_size - 1);
        table[idx] = h;
    }

    std::ofstream f(path, std::ios::binary);
    const uint8_t magic[4] = {'C', 'L', 'A', 'V'};
    f.write(reinterpret_cast<const char*>(magic), 4);
    const uint32_t cnt = static_cast<uint32_t>(count);
    f.write(reinterpret_cast<const char*>(&cnt), 4);
    f.write(reinterpret_cast<const char*>(table.data()),
            static_cast<std::streamsize>(table_size * 8));
    return path;
}

} // namespace

TEST_CASE("Dictionary: loads from valid binary", "[dictionary]") {
    const auto path = write_temp_dict({"hello", "world"});
    clavi::Dictionary dict;
    REQUIRE(dict.load(path));
    REQUIRE_FALSE(dict.empty());
}

TEST_CASE("Dictionary: fails on nonexistent file", "[dictionary]") {
    clavi::Dictionary dict;
    REQUIRE_FALSE(dict.load("/no/such/file.bin"));
    REQUIRE(dict.empty());
}

TEST_CASE("Dictionary: known English words found", "[dictionary]") {
    const auto path = write_temp_dict({"hello", "world", "keyboard", "language"});
    clavi::Dictionary dict;
    REQUIRE(dict.load(path));

    REQUIRE(dict.contains("hello"));
    REQUIRE(dict.contains("world"));
    REQUIRE(dict.contains("keyboard"));
    REQUIRE(dict.contains("language"));
}

TEST_CASE("Dictionary: unknown words not found", "[dictionary]") {
    const auto path = write_temp_dict({"hello", "world"});
    clavi::Dictionary dict;
    REQUIRE(dict.load(path));

    REQUIRE_FALSE(dict.contains("xkcd1234garbage"));
    REQUIRE_FALSE(dict.contains("qqqqqqqq"));
    REQUIRE_FALSE(dict.contains("привіт"));
}

TEST_CASE("Dictionary: known Ukrainian words found", "[dictionary]") {
    const auto path = write_temp_dict({"привіт", "клавіатура", "мова"});
    clavi::Dictionary dict;
    REQUIRE(dict.load(path));

    REQUIRE(dict.contains("привіт"));
    REQUIRE(dict.contains("клавіатура"));
    REQUIRE(dict.contains("мова"));
}

TEST_CASE("Dictionary: case-insensitive lookup", "[dictionary]") {
    const auto path = write_temp_dict({"hello"});
    clavi::Dictionary dict;
    REQUIRE(dict.load(path));

    REQUIRE(dict.contains("hello"));
    REQUIRE(dict.contains("Hello"));
    REQUIRE(dict.contains("HELLO"));
}

TEST_CASE("Dictionary: rejects bad magic", "[dictionary]") {
    std::string path = std::filesystem::temp_directory_path().string() + "/bad_dict.bin";
    std::ofstream f(path, std::ios::binary);
    const uint8_t bad[8] = {'X', 'X', 'X', 'X', 0, 0, 0, 0};
    f.write(reinterpret_cast<const char*>(bad), 8);
    f.close();

    clavi::Dictionary dict;
    REQUIRE_FALSE(dict.load(path));
}
