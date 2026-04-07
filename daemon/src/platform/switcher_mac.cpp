#include "clavi/platform/switcher.hpp"

#include <Carbon/Carbon.h>
#include <cstdlib>
#include <string>

namespace clavi {

namespace {

class SwitcherMac final : public ISwitcher {
public:
    bool switch_layout(std::string_view locale) override {
        // Map locale to macOS input source ID
        const char* source_id = nullptr;
        if (locale == "uk")      source_id = "com.apple.keylayout.Ukrainian-PC";
        else if (locale == "en") source_id = "com.apple.keylayout.US";
        else return false;

        // Use AppleScript to switch input source
        const std::string script = std::string(
            "tell application \"System Events\" to "
            "tell process \"SystemUIServer\" to "
            "set value of attribute \"AXMenuItemMarkChar\" of "
            "menu item \"") + source_id + "\" of menu 1 of menu bar item "
            "\"Input menu\" of menu bar 1 to \"✓\"";
        // Simpler approach: use TIS API
        CFStringRef src_id_cf = CFStringCreateWithCString(
            kCFAllocatorDefault, source_id, kCFStringEncodingUTF8);
        if (!src_id_cf) return false;

        TISInputSourceRef source = TISCopyInputSourceForLanguage(
            CFStringCreateWithCString(kCFAllocatorDefault,
                locale.data(), kCFStringEncodingUTF8));
        if (source) {
            TISSelectInputSource(source);
            CFRelease(source);
        }
        CFRelease(src_id_cf);
        return source != nullptr;
    }

    void retype(std::size_t char_count, std::string_view text) override {
        if (char_count > 0) {
            // Send Backspace × N via osascript
            const std::string bs_script =
                "tell application \"System Events\" to repeat " +
                std::to_string(char_count) +
                " times\nkey code 51\nend repeat\nend tell";
            const std::string cmd = std::string("osascript -e '") + bs_script + "' 2>/dev/null";
            std::system(cmd.c_str());
        }
        if (text.empty()) return;

        // Type via osascript keystroke
        std::string safe;
        safe.reserve(text.size());
        for (char c : text) {
            if (c == '"') safe += "\\\"";
            else if (c == '\\') safe += "\\\\";
            else safe += c;
        }
        const std::string cmd =
            std::string("osascript -e 'tell application \"System Events\" to keystroke \"") +
            safe + "\"' 2>/dev/null";
        std::system(cmd.c_str());
    }
};

} // namespace

std::unique_ptr<ISwitcher> ISwitcher::create() {
    return std::make_unique<SwitcherMac>();
}

} // namespace clavi
