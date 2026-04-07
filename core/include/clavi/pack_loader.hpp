#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace clavi {

struct PackInfo {
    std::string locale;
    std::string name;
    std::string version;
    bool feature_switch{false};
    bool feature_translit{false};
    bool feature_bridge{false};
    // file references (relative to pack directory)
    std::string file_keyboard_map;
    std::string file_dictionary;
    std::string file_ngram;
    std::string file_translit;
};

class PackLoader {
public:
    [[nodiscard]] static bool is_allowed(std::string_view locale) noexcept;

    // Parse pack.toml and return structured info.  Returns nullopt on failure.
    [[nodiscard]] static std::optional<PackInfo> load_pack_info(
        std::string_view pack_toml_path) noexcept;

private:
    // HARDCODED — do not make this configurable
    static constexpr std::array<std::string_view, 1> BLOCKED_LOCALES = {"ru"};
};

} // namespace clavi
