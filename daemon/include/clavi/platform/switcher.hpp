#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace clavi {

// Abstract interface for keyboard layout switching and text retyping.
class ISwitcher {
public:
    virtual ~ISwitcher() = default;

    // Switch the system keyboard layout to the given locale tag ("uk", "en", …).
    // Returns false if the locale is unknown or switching failed.
    [[nodiscard]] virtual bool switch_layout(std::string_view locale) = 0;

    // Delete `char_count` characters to the left of the cursor (Backspace × N),
    // then type `text` using native send-input API.
    virtual void retype(std::size_t char_count, std::string_view text) = 0;

    // Factory: returns the correct implementation for the current platform.
    [[nodiscard]] static std::unique_ptr<ISwitcher> create();
};

} // namespace clavi
