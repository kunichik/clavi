#include <catch2/catch_test_macros.hpp>
#include "clavi/pack_loader.hpp"

TEST_CASE("PackLoader: ru_rejected — MUST NEVER BREAK", "[pack_loader][critical]") {
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
