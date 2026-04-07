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

TEST_CASE("Config: default translit_locale is uk", "[config]") {
    const auto cfg = clavi::Config::load_defaults();
    REQUIRE(cfg.general.translit_locale == "uk");
}

TEST_CASE("Config: parses translit_locale from TOML", "[config]") {
    const auto content = R"(
[general]
translit_locale = "pl"
)";
    const auto path = write_temp_config(content, "test_translit_locale.toml");
    const auto cfg = clavi::Config::load(path);
    REQUIRE(cfg.general.translit_locale == "pl");
}

TEST_CASE("Config: default mode is detection", "[config]") {
    const auto cfg = clavi::Config::load_defaults();
    REQUIRE(cfg.general.mode == "detection");
}

TEST_CASE("Config: parses mode = bridge from TOML", "[config]") {
    const auto content = R"(
[general]
mode = "bridge"
translit_locale = "uk"
)";
    const auto path = write_temp_config(content, "test_bridge_mode.toml");
    const auto cfg = clavi::Config::load(path);
    REQUIRE(cfg.general.mode == "bridge");
    REQUIRE(cfg.general.translit_locale == "uk");
}

TEST_CASE("Config: invalid TOML returns defaults without crash", "[config]") {
    const auto path = write_temp_config("not valid [[[ toml content", "bad_config.toml");
    const auto cfg = clavi::Config::load(path);
    // Must not crash, must return defaults
    REQUIRE(cfg.general.enabled == true);
}

// ── Config::validate() tests ─────────────────────────────────────────────────

TEST_CASE("Config: defaults pass validation", "[config][validate]") {
    const auto cfg = clavi::Config::load_defaults();
    REQUIRE(cfg.validate().empty());
}

TEST_CASE("Config: invalid mode caught by validate", "[config][validate]") {
    auto cfg = clavi::Config::load_defaults();
    cfg.general.mode = "bogus";
    const auto errs = cfg.validate();
    REQUIRE(errs.size() == 1);
    REQUIRE(errs[0].find("general.mode") != std::string::npos);
}

TEST_CASE("Config: invalid log level caught by validate", "[config][validate]") {
    auto cfg = clavi::Config::load_defaults();
    cfg.logging.level = "verbose";
    const auto errs = cfg.validate();
    REQUIRE(errs.size() == 1);
    REQUIRE(errs[0].find("logging.level") != std::string::npos);
}

TEST_CASE("Config: layer2_threshold out of range", "[config][validate]") {
    auto cfg = clavi::Config::load_defaults();
    cfg.detection.layer2_threshold = 1.5;
    const auto errs = cfg.validate();
    REQUIRE(errs.size() == 1);
    REQUIRE(errs[0].find("layer2_threshold") != std::string::npos);
}

TEST_CASE("Config: empty active_pair caught", "[config][validate]") {
    auto cfg = clavi::Config::load_defaults();
    cfg.general.active_pair.clear();
    const auto errs = cfg.validate();
    REQUIRE(errs.size() == 1);
    REQUIRE(errs[0].find("active_pair") != std::string::npos);
}

TEST_CASE("Config: invalid exclusion match type", "[config][validate]") {
    auto cfg = clavi::Config::load_defaults();
    cfg.exclusions.match = "glob";
    const auto errs = cfg.validate();
    REQUIRE(errs.size() == 1);
    REQUIRE(errs[0].find("exclusions.match") != std::string::npos);
}

TEST_CASE("Config: multiple errors reported at once", "[config][validate]") {
    auto cfg = clavi::Config::load_defaults();
    cfg.general.mode = "invalid";
    cfg.logging.level = "invalid";
    cfg.detection.layer2_threshold = -0.5;
    const auto errs = cfg.validate();
    REQUIRE(errs.size() == 3);
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
