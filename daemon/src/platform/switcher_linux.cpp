#include "clavi/platform/switcher.hpp"

#include <cstdlib>
#include <cstring>
#include <string>

namespace clavi {

namespace {

class SwitcherLinux final : public ISwitcher {
public:
    bool switch_layout(std::string_view locale) override {
        // Map locale tag to XKB layout name
        const char* xkb = nullptr;
        if (locale == "uk") xkb = "ua";
        else if (locale == "en") xkb = "us";
        else return false;

        // Try gsettings (GNOME) first, fall back to setxkbmap
        std::string cmd = "gsettings set org.gnome.desktop.input-sources current-input-source ";
        // GNOME input-source index is not trivially known — use setxkbmap as universal fallback
        cmd = std::string("setxkbmap ") + xkb + " 2>/dev/null";
        return std::system(cmd.c_str()) == 0;
    }

    void retype(std::size_t char_count, std::string_view text) override {
        if (char_count > 0) {
            // Delete char_count characters via xdotool
            for (std::size_t i = 0; i < char_count; ++i) {
                std::system("xdotool key BackSpace 2>/dev/null");
            }
        }
        if (text.empty()) return;

        // Type new text via xdotool
        // Escape single quotes in text
        std::string safe;
        safe.reserve(text.size() + 2);
        for (char c : text) {
            if (c == '\'') safe += "'\\''";
            else safe += c;
        }
        const std::string cmd = std::string("xdotool type --clearmodifiers '") + safe + "' 2>/dev/null";
        std::system(cmd.c_str());
    }
};

} // namespace

std::unique_ptr<ISwitcher> ISwitcher::create() {
    return std::make_unique<SwitcherLinux>();
}

} // namespace clavi
