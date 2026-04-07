#include <catch2/catch_test_macros.hpp>
#include "clavi/pack_loader.hpp"

#include <filesystem>
#include <fstream>

TEST_CASE("PackLoader: ru_rejected (CRITICAL - MUST NEVER BREAK)", "[pack_loader][critical]") {
    REQUIRE_FALSE(clavi::PackLoader::is_allowed("ru"));
}

TEST_CASE("PackLoader: uk allowed", "[pack_loader]") {
    REQUIRE(clavi::PackLoader::is_allowed("uk"));
}

TEST_CASE("PackLoader: en allowed", "[pack_loader]") {
    REQUIRE(clavi::PackLoader::is_allowed("en"));
}

TEST_CASE("PackLoader: empty string allowed (handled by caller)", "[pack_loader]") {
    REQUIRE(clavi::PackLoader::is_allowed(""));
}

TEST_CASE("PackLoader: other locales allowed", "[pack_loader]") {
    REQUIRE(clavi::PackLoader::is_allowed("de"));
    REQUIRE(clavi::PackLoader::is_allowed("fr"));
    REQUIRE(clavi::PackLoader::is_allowed("es"));
    REQUIRE(clavi::PackLoader::is_allowed("ko"));
    REQUIRE(clavi::PackLoader::is_allowed("ja"));
    REQUIRE(clavi::PackLoader::is_allowed("zh"));
}

TEST_CASE("PackLoader: ru_ prefix or suffix NOT blocked", "[pack_loader]") {
    // Only exact "ru" is blocked, not locales that contain "ru" as substring
    REQUIRE(clavi::PackLoader::is_allowed("ru_UA"));
    REQUIRE(clavi::PackLoader::is_allowed("xru"));
}

// ── load_pack_info tests ─────────────────────────────────────────────────────

namespace {
std::string write_temp_pack_toml(const std::string& content,
                                  const std::string& name = "test_pack.toml") {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream(path) << content;
    return path.string();
}
} // namespace

TEST_CASE("PackLoader: load_pack_info parses valid pack.toml", "[pack_loader]") {
    const auto path = write_temp_pack_toml(R"(
[pack]
locale = "uk"
name = "Ukrainian"
version = "1.0.0"

[features]
switch = true
translit = true
bridge = true

[files]
keyboard_map = "keyboard_map.bin"
dictionary = "dictionary.bin"
ngram = "ngram.bin"
translit = "translit.toml"
)");
    const auto info = clavi::PackLoader::load_pack_info(path);
    REQUIRE(info.has_value());
    REQUIRE(info->locale == "uk");
    REQUIRE(info->name == "Ukrainian");
    REQUIRE(info->version == "1.0.0");
    REQUIRE(info->feature_switch == true);
    REQUIRE(info->feature_translit == true);
    REQUIRE(info->feature_bridge == true);
    REQUIRE(info->file_keyboard_map == "keyboard_map.bin");
    REQUIRE(info->file_dictionary == "dictionary.bin");
    REQUIRE(info->file_ngram == "ngram.bin");
    REQUIRE(info->file_translit == "translit.toml");
}

TEST_CASE("PackLoader: load_pack_info returns nullopt for missing file", "[pack_loader]") {
    const auto info = clavi::PackLoader::load_pack_info("/nonexistent/pack.toml");
    REQUIRE_FALSE(info.has_value());
}

TEST_CASE("PackLoader: load_pack_info returns nullopt when locale empty", "[pack_loader]") {
    const auto path = write_temp_pack_toml(R"(
[pack]
name = "No locale"
)", "test_no_locale.toml");
    const auto info = clavi::PackLoader::load_pack_info(path);
    REQUIRE_FALSE(info.has_value());
}

TEST_CASE("PackLoader: load_pack_info defaults features to false", "[pack_loader]") {
    const auto path = write_temp_pack_toml(R"(
[pack]
locale = "de"
name = "German"
)", "test_de_pack.toml");
    const auto info = clavi::PackLoader::load_pack_info(path);
    REQUIRE(info.has_value());
    REQUIRE(info->locale == "de");
    REQUIRE(info->feature_switch == false);
    REQUIRE(info->feature_translit == false);
    REQUIRE(info->feature_bridge == false);
    REQUIRE(info->file_dictionary.empty());
}
