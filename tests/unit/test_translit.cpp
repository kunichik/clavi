#include <catch2/catch_test_macros.hpp>
#include "clavi/translit.hpp"

#include <filesystem>
#include <fstream>

namespace {
std::string write_translit_toml(std::string_view content) {
    const auto path = (std::filesystem::temp_directory_path() / "test_translit.toml").string();
    std::ofstream f(path);
    f << content;
    return path;
}
} // namespace

TEST_CASE("Translit: empty on init", "[translit]") {
    clavi::Translit tr;
    REQUIRE(tr.empty());
}

TEST_CASE("Translit: loads rules from TOML", "[translit]") {
    const auto path = write_translit_toml(R"(
[rules]
"a" = "а"
"b" = "б"
"v" = "в"
)");
    clavi::Translit tr;
    REQUIRE(tr.load(path));
    REQUIRE_FALSE(tr.empty());
}

TEST_CASE("Translit: simple single-char rules", "[translit]") {
    const auto path = write_translit_toml(R"(
[rules]
"a" = "а"
"b" = "б"
"v" = "в"
"h" = "г"
"d" = "д"
)");
    clavi::Translit tr;
    REQUIRE(tr.load(path));

    REQUIRE(tr.transliterate("a") == "а");
    REQUIRE(tr.transliterate("b") == "б");
    REQUIRE(tr.transliterate("abvhd") == "абвгд");
}

TEST_CASE("Translit: digraph rules take priority over single-char", "[translit]") {
    const auto path = write_translit_toml(R"(
[rules]
"z" = "з"
"zh" = "ж"
"h" = "г"
"s" = "с"
"sh" = "ш"
)");
    clavi::Translit tr;
    REQUIRE(tr.load(path));

    REQUIRE(tr.transliterate("zh") == "ж");
    REQUIRE(tr.transliterate("sh") == "ш");
    REQUIRE(tr.transliterate("z") == "з");
}

TEST_CASE("Translit: passthrough for unmapped characters", "[translit]") {
    const auto path = write_translit_toml(R"(
[rules]
"a" = "а"
)");
    clavi::Translit tr;
    REQUIRE(tr.load(path));

    REQUIRE(tr.transliterate("xyzabc") == "xyzаbc");
}

TEST_CASE("Translit: returns input unchanged if empty rules", "[translit]") {
    clavi::Translit tr;
    REQUIRE(tr.transliterate("hello") == "hello");
}

TEST_CASE("Translit: missing file returns false", "[translit]") {
    clavi::Translit tr;
    REQUIRE_FALSE(tr.load("/no/such/file.toml"));
    REQUIRE(tr.empty());
}
