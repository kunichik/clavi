#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "clavi/ngram_model.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string test_data_dir() {
    // Relative to CWD (project root when running via CTest)
    return "packs";
}

// Create a minimal ngram.bin in-memory and write to a temp file.
// ngrams: sorted vector of {3-byte ngram, float log_prob}
static std::string write_temp_ngram(
    const std::vector<std::pair<std::string, float>>& entries,
    uint8_t n = 3)
{
    static int counter = 0;
    const std::string path =
        std::string("test_ngram_") + std::to_string(counter++) + ".bin";

    std::ofstream f(path, std::ios::binary);
    f.write("NGRM", 4);
    const uint32_t count = static_cast<uint32_t>(entries.size());
    f.write(reinterpret_cast<const char*>(&count), 4);
    f.write(reinterpret_cast<const char*>(&n), 1);
    for (const auto& [ng, lp] : entries) {
        f.write(ng.data(), n);
        f.write(reinterpret_cast<const char*>(&lp), 4);
    }
    f.close();
    return path;
}

static void cleanup_file(const std::string& path) {
    std::remove(path.c_str());
}

// ── Tests ─────────────────────────────────────────────────────────────────────

TEST_CASE("NgramModel: empty by default", "[ngram]") {
    clavi::NgramModel model;
    REQUIRE(!model.loaded());
    REQUIRE(model.size() == 0);
    REQUIRE(!model.score("hello").has_value());
}

TEST_CASE("NgramModel: load rejects bad magic", "[ngram]") {
    const std::string path = "test_bad_magic.bin";
    {
        std::ofstream f(path, std::ios::binary);
        f.write("XXXX", 4);
        uint32_t count = 0;
        f.write(reinterpret_cast<const char*>(&count), 4);
        uint8_t n = 3;
        f.write(reinterpret_cast<const char*>(&n), 1);
    }
    clavi::NgramModel model;
    REQUIRE(!model.load(path));
    cleanup_file(path);
}

TEST_CASE("NgramModel: load minimal valid file", "[ngram]") {
    // Two sorted 3-byte trigrams
    const auto path = write_temp_ngram({
        {"aaa", -2.0f},
        {"bbb", -3.0f},
    });
    clavi::NgramModel model;
    REQUIRE(model.load(path));
    REQUIRE(model.loaded());
    REQUIRE(model.size() == 2);
    REQUIRE(model.ngram_size() == 3);
    cleanup_file(path);
}

TEST_CASE("NgramModel: score short text returns nullopt", "[ngram]") {
    const auto path = write_temp_ngram({{"abc", -1.0f}});
    clavi::NgramModel model;
    REQUIRE(model.load(path));
    // "ab" is 2 bytes, less than ngram_size=3
    REQUIRE(!model.score("ab").has_value());
    cleanup_file(path);
}

TEST_CASE("NgramModel: score returns average log-prob", "[ngram]") {
    // "abc" → one 3-gram "abc" with log_prob -2.0
    // "abcd" → two 3-grams: "abc" (-2.0) and "bcd" (unseen → -10.0)
    const auto path = write_temp_ngram({{"abc", -2.0f}});
    clavi::NgramModel model;
    REQUIRE(model.load(path));

    // Exact match: "abc" → average of [-2.0] = -2.0
    auto s1 = model.score("abc");
    REQUIRE(s1.has_value());
    REQUIRE_THAT(*s1, Catch::Matchers::WithinAbs(-2.0, 0.01));

    // Two grams: "abc"(-2.0) + "bcd"(-10.0) → average = -6.0
    auto s2 = model.score("abcd");
    REQUIRE(s2.has_value());
    REQUIRE_THAT(*s2, Catch::Matchers::WithinAbs(-6.0, 0.01));

    cleanup_file(path);
}

TEST_CASE("NgramModel: unsorted file rejected", "[ngram]") {
    const auto path = write_temp_ngram({
        {"zzz", -1.0f},
        {"aaa", -2.0f}, // out of order
    });
    clavi::NgramModel model;
    REQUIRE(!model.load(path));
    cleanup_file(path);
}

TEST_CASE("NgramModel: load real uk pack", "[ngram][integration]") {
    const std::string path = test_data_dir() + "/uk/ngram.bin";
    clavi::NgramModel model;
    if (!model.load(path)) {
        WARN("Skipping: uk ngram.bin not found at " + path);
        return;
    }
    REQUIRE(model.loaded());
    REQUIRE(model.ngram_size() == 3);
    REQUIRE(model.size() > 100);

    // Ukrainian word should score reasonably well
    auto s_uk = model.score("\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82"); // привіт
    REQUIRE(s_uk.has_value());

    // English word should score worse (more unseen byte trigrams)
    auto s_en = model.score("hello");
    REQUIRE(s_en.has_value());

    // Ukrainian text should score higher (less negative) on uk model
    REQUIRE(*s_uk > *s_en);
}

TEST_CASE("NgramModel: load real en pack", "[ngram][integration]") {
    const std::string path = test_data_dir() + "/en/ngram.bin";
    clavi::NgramModel model;
    if (!model.load(path)) {
        WARN("Skipping: en ngram.bin not found at " + path);
        return;
    }
    REQUIRE(model.loaded());
    REQUIRE(model.ngram_size() == 3);
    REQUIRE(model.size() > 100);

    // English word should score well
    auto s_en = model.score("hello");
    REQUIRE(s_en.has_value());

    // Ukrainian word should score worse
    auto s_uk = model.score("\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82"); // привіт
    REQUIRE(s_uk.has_value());

    REQUIRE(*s_en > *s_uk);
}

TEST_CASE("NgramModel: cross-model discrimination", "[ngram][integration]") {
    const std::string uk_path = test_data_dir() + "/uk/ngram.bin";
    const std::string en_path = test_data_dir() + "/en/ngram.bin";

    clavi::NgramModel uk_model, en_model;
    if (!uk_model.load(uk_path) || !en_model.load(en_path)) {
        WARN("Skipping: ngram.bin not found");
        return;
    }

    // привіт should score higher on uk model than en model
    const auto pryvit = "\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82";
    auto uk_on_uk = uk_model.score(pryvit);
    auto uk_on_en = en_model.score(pryvit);
    REQUIRE(uk_on_uk.has_value());
    REQUIRE(uk_on_en.has_value());
    REQUIRE(*uk_on_uk > *uk_on_en);

    // "keyboard" should score higher on en model than uk model
    auto en_on_en = en_model.score("keyboard");
    auto en_on_uk = uk_model.score("keyboard");
    REQUIRE(en_on_en.has_value());
    REQUIRE(en_on_uk.has_value());
    REQUIRE(*en_on_en > *en_on_uk);
}
