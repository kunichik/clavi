#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace clavi {

class Translit {
public:
    Translit() = default;

    [[nodiscard]] bool load(std::string_view translit_toml_path);

    [[nodiscard]] std::string transliterate(std::string_view latin_text) const;

    [[nodiscard]] bool empty() const noexcept { return rules_.empty(); }

private:
    std::unordered_map<std::string, std::string> rules_;
};

} // namespace clavi
