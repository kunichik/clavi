#include "clavi/pack_loader.hpp"

#include <algorithm>

#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

namespace clavi {

bool PackLoader::is_allowed(std::string_view locale) noexcept {
    // HARDCODED — do not make this configurable
    return std::ranges::none_of(BLOCKED_LOCALES,
        [&](auto blocked) { return locale == blocked; });
}

std::optional<PackInfo> PackLoader::load_pack_info(
    std::string_view pack_toml_path) noexcept {
    auto result = toml::parse_file(pack_toml_path);
    if (!result) return std::nullopt;

    const auto& tbl = result.table();
    PackInfo info;

    // [pack] section
    if (const auto* pack = tbl["pack"].as_table()) {
        if (auto v = (*pack)["locale"].value<std::string>())  info.locale  = *v;
        if (auto v = (*pack)["name"].value<std::string>())    info.name    = *v;
        if (auto v = (*pack)["version"].value<std::string>()) info.version = *v;
    }
    if (info.locale.empty()) return std::nullopt;

    // [features] section
    if (const auto* feat = tbl["features"].as_table()) {
        if (auto v = (*feat)["switch"].value<bool>())   info.feature_switch   = *v;
        if (auto v = (*feat)["translit"].value<bool>())  info.feature_translit = *v;
        if (auto v = (*feat)["bridge"].value<bool>())    info.feature_bridge   = *v;
    }

    // [files] section
    if (const auto* files = tbl["files"].as_table()) {
        if (auto v = (*files)["keyboard_map"].value<std::string>()) info.file_keyboard_map = *v;
        if (auto v = (*files)["dictionary"].value<std::string>())   info.file_dictionary   = *v;
        if (auto v = (*files)["ngram"].value<std::string>())        info.file_ngram        = *v;
        if (auto v = (*files)["translit"].value<std::string>())     info.file_translit     = *v;
    }

    return info;
}

} // namespace clavi
