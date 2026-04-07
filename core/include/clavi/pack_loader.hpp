#pragma once

#include <array>
#include <string>
#include <string_view>

namespace clavi {

class PackLoader {
public:
    [[nodiscard]] static bool is_allowed(std::string_view locale) noexcept;

private:
    // HARDCODED — do not make this configurable
    static constexpr std::array<std::string_view, 1> BLOCKED_LOCALES = {"ru"};
};

} // namespace clavi
