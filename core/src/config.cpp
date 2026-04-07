#include "clavi/config.hpp"

#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

namespace clavi {

Config Config::load_defaults() noexcept {
    return Config{};
}

Config Config::load(std::string_view config_path,
                    std::string_view exclusions_path) noexcept {
    Config cfg = load_defaults();

    // Main config
    if (!config_path.empty()) {
        auto result = toml::parse_file(config_path);
        if (result) {
            const auto& tbl = result.table();

            if (const auto* general = tbl["general"].as_table()) {
                if (auto v = (*general)["enabled"].value<bool>()) cfg.general.enabled = *v;
                if (auto v = (*general)["min_word_length"].value<int64_t>())
                    cfg.general.min_word_length = static_cast<int>(*v);
                if (const auto* pair = (*general)["active_pair"].as_array()) {
                    cfg.general.active_pair.clear();
                    for (const auto& el : *pair) {
                        if (auto s = el.value<std::string>())
                            cfg.general.active_pair.push_back(*s);
                    }
                }
                if (auto v = (*general)["translit_locale"].value<std::string>())
                    cfg.general.translit_locale = *v;
                if (auto v = (*general)["mode"].value<std::string>())
                    cfg.general.mode = *v;
            }

            if (const auto* hotkeys = tbl["hotkeys"].as_table()) {
                if (auto v = (*hotkeys)["toggle"].value<std::string>()) cfg.hotkeys.toggle = *v;
                if (auto v = (*hotkeys)["undo"].value<std::string>()) cfg.hotkeys.undo = *v;
            }

            if (const auto* detection = tbl["detection"].as_table()) {
                if (auto v = (*detection)["layer2_threshold"].value<double>())
                    cfg.detection.layer2_threshold = *v;
                if (auto v = (*detection)["layer3_timeout_ms"].value<int64_t>())
                    cfg.detection.layer3_timeout_ms = static_cast<int>(*v);
                if (auto v = (*detection)["layer3_enabled"].value<bool>())
                    cfg.detection.layer3_enabled = *v;
            }

            if (const auto* logging = tbl["logging"].as_table()) {
                if (auto v = (*logging)["enabled"].value<bool>()) cfg.logging.enabled = *v;
                if (auto v = (*logging)["level"].value<std::string>()) cfg.logging.level = *v;
            }
        }
        // If parse fails, silently use defaults (no crash)
    }

    // Exclusions config
    if (!exclusions_path.empty()) {
        auto result = toml::parse_file(exclusions_path);
        if (result) {
            const auto& tbl = result.table();

            if (const auto* words = tbl["words"].as_table()) {
                if (const auto* skip = (*words)["skip"].as_array()) {
                    for (const auto& el : *skip) {
                        if (auto s = el.value<std::string>())
                            cfg.exclusions.skip_words.push_back(*s);
                    }
                }
            }

            if (const auto* apps = tbl["apps"].as_table()) {
                if (const auto* skip = (*apps)["skip"].as_array()) {
                    for (const auto& el : *skip) {
                        if (auto s = el.value<std::string>())
                            cfg.exclusions.skip_apps.push_back(*s);
                    }
                }
                if (auto v = (*apps)["match"].value<std::string>())
                    cfg.exclusions.match = *v;
            }
        }
    }

    return cfg;
}

std::vector<std::string> Config::validate() const noexcept {
    std::vector<std::string> errors;

    // mode
    if (general.mode != "detection" && general.mode != "bridge")
        errors.push_back("general.mode: expected 'detection' or 'bridge', got '" +
                         general.mode + "'");

    // active_pair
    if (general.active_pair.empty())
        errors.push_back("general.active_pair: must contain at least one locale");

    // min_word_length
    if (general.min_word_length < 1)
        errors.push_back("general.min_word_length: must be >= 1");

    // logging level
    const auto& lvl = logging.level;
    if (lvl != "debug" && lvl != "info" && lvl != "warn" && lvl != "error")
        errors.push_back("logging.level: expected debug|info|warn|error, got '" +
                         lvl + "'");

    // detection thresholds
    if (detection.layer2_threshold < 0.0 || detection.layer2_threshold > 1.0)
        errors.push_back("detection.layer2_threshold: must be in range [0.0, 1.0]");
    if (detection.layer3_timeout_ms < 0)
        errors.push_back("detection.layer3_timeout_ms: must be >= 0");

    // exclusion match type
    const auto& m = exclusions.match;
    if (m != "exact" && m != "substring" && m != "regex")
        errors.push_back("exclusions.match: expected exact|substring|regex, got '" +
                         m + "'");

    return errors;
}

} // namespace clavi
