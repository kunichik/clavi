#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace clavi {

struct HotkeyConfig {
    std::string toggle{"Ctrl+Shift+Space"};
    std::string undo{"Ctrl+Z"};
};

struct DetectionConfig {
    double layer2_threshold{0.75};
    int layer3_timeout_ms{150};
    bool layer3_enabled{false};
};

struct LoggingConfig {
    bool enabled{false};
    std::string level{"info"};
};

struct GeneralConfig {
    bool enabled{true};
    std::vector<std::string> active_pair{"uk", "en"};
    int min_word_length{3};
    std::string translit_locale{"uk"}; // target locale for Ctrl+T translit mode
    // "detection" = auto-detect wrong layout (default)
    // "bridge"    = always-on translit: type Latin, output Cyrillic, no layout switch
    std::string mode{"detection"};
};

struct ExclusionConfig {
    std::vector<std::string> skip_words;
    std::vector<std::string> skip_apps;
    std::string match{"substring"};
};

struct Config {
    GeneralConfig general;
    HotkeyConfig hotkeys;
    DetectionConfig detection;
    LoggingConfig logging;
    ExclusionConfig exclusions;

    [[nodiscard]] static Config load_defaults() noexcept;
    [[nodiscard]] static Config load(std::string_view config_path,
                                     std::string_view exclusions_path = "") noexcept;
};

} // namespace clavi
