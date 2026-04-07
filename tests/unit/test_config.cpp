#include <catch2/catch_test_macros.hpp>
#include "clavi/config.hpp"

#include <filesystem>
#include <fstream>

namespace {
std::string write_temp_config(std::string_view contents, std::string_view filename) {
    const auto path = (std::filesystem::temp_directory_path() / filename).string();
    std::ofstream f(path);
    f << contents;
    return path;
}
} // namespace

TEST_CASE("Config: missing file returns defaults without crash", "[config]") {
    const auto cfg = clavi::Config::load("/nonexistent/config.toml");
    REQUIRE(cfg.general.enabled == true);
    REQUIRE(cfg.general.min_word_length == 3);
    REQUIRE(cfg.hotkeys.toggle == "Ctrl+Shift+Space");
    REQUIRE(cfg.hotkeys.undo == "Ctrl+Z");
    REQUIRE(cfg.detection.layer2_threshold == 0.75);
    REQUIRE(cfg.detection.layer3_timeout_ms == 150);
    REQUIRE(cfg.detection.layer3_enabled == false);
    REQUIRE(cfg.logging.enabled == false);
    REQUIRE(cfg.logging.level == "info");
}

TEST_CASE("Config: default active_pair is uk+en", "[config]") {
    const auto cfg = clavi::Config::load_defaults();
    REQUIRE(cfg.general.active_pair.size() == 2);
    REQUIRE(cfg.general.active_pair[0] == "uk");
    REQUIRE(cfg.general.active_pair[1] == "en");
}

TEST_CASE("Config: parses valid config.toml", "[config]") {
    const auto content = R"(
[general]
enabled = false
min_word_length = 4
active_pair = ["en", "de"]

[hotkeys]
toggle = "Ctrl+Alt+Space"
undo = "Ctrl+Alt+Z"

[detection]
layer2_threshold = 0.85
layer3_timeout_ms = 200
layer3_enabled = true

[logging]
enabled = true
level = "debug"
)";
    const auto path = write_temp_config(content, "test_config.toml");
    const auto cfg = clavi::Config::load(path);

    REQUIRE(cfg.general.enabled == false);
    REQUIRE(cfg.general.min_word_length == 4);
    REQUIRE(cfg.general.active_pair.size() == 2);
    REQUIRE(cfg.general.active_pair[0] == "en");
    REQUIRE(cfg.general.active_pair[1] == "de");
    REQUIRE(cfg.hotkeys.toggle == "Ctrl+Alt+Space");
    REQUIRE(cfg.hotkeys.undo == "Ctrl+Alt+Z");
    REQUIRE(cfg.detection.layer2_threshold == 0.85);
    REQUIRE(cfg.detection.layer3_timeout_ms == 200);
    REQUIRE(cfg.detection.layer3_enabled == true);
    REQUIRE(cfg.logging.enabled == true);
    REQUIRE(cfg.logging.level == "debug");
}

TEST_CASE("Config: invalid TOML returns defaults without crash", "[config]") {
    const auto path = write_temp_config("not valid [[[ toml content", "bad_config.toml");
    const auto cfg = clavi::Config::load(path);
    // Must not crash, must return defaults
    REQUIRE(cfg.general.enabled == true);
}

TEST_CASE("Config: parses exclusions.toml", "[config]") {
    const auto excl = R"(
[words]
skip = ["git", "npm", "sudo"]

[apps]
skip = ["terminal", "code"]
match = "exact"
)";
    const auto path = write_temp_config(excl, "test_excl.toml");
    const auto cfg = clavi::Config::load("", path);

    REQUIRE(cfg.exclusions.skip_words.size() == 3);
    REQUIRE(cfg.exclusions.skip_words[0] == "git");
    REQUIRE(cfg.exclusions.skip_apps.size() == 2);
    REQUIRE(cfg.exclusions.match == "exact");
}
