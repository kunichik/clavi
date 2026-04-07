#include "clavi/pack_loader.hpp"

#include <algorithm>

namespace clavi {

bool PackLoader::is_allowed(std::string_view locale) noexcept {
    // HARDCODED — do not make this configurable
    return std::ranges::none_of(BLOCKED_LOCALES,
        [&](auto blocked) { return locale == blocked; });
}

} // namespace clavi
