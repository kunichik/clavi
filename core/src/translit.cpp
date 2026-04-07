#include "clavi/translit.hpp"

#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

namespace clavi {

bool Translit::load(std::string_view path) {
    auto result = toml::parse_file(path);
    if (!result) return false;

    const auto& tbl = result.table();
    if (const auto* rules = tbl["rules"].as_table()) {
        for (const auto& [k, v] : *rules) {
            if (auto s = v.value<std::string>()) {
                rules_[std::string(k.str())] = *s;
            }
        }
    }
    return !rules_.empty();
}

std::string Translit::transliterate(std::string_view latin_text) const {
    if (rules_.empty()) return std::string(latin_text);

    std::string result;
    result.reserve(latin_text.size() * 2);

    std::size_t i = 0;
    while (i < latin_text.size()) {
        bool matched = false;
        // Try longest match first (up to 3 chars for digraphs like "zh", "ch", "shch")
        for (int len = 4; len >= 1; --len) {
            if (i + static_cast<std::size_t>(len) > latin_text.size()) continue;
            const std::string key(latin_text.substr(i, static_cast<std::size_t>(len)));
            auto it = rules_.find(key);
            if (it != rules_.end()) {
                result += it->second;
                i += static_cast<std::size_t>(len);
                matched = true;
                break;
            }
        }
        if (!matched) {
            result += latin_text[i];
            ++i;
        }
    }
    return result;
}

} // namespace clavi
