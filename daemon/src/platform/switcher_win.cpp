#include "clavi/platform/switcher.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <imm.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace clavi {

namespace {

// HKL identifiers for common keyboard layouts
static const std::unordered_map<std::string, LANGID> LOCALE_TO_LANGID = {
    {"uk", MAKELANGID(LANG_UKRAINIAN, SUBLANG_UKRAINIAN_UKRAINE)},  // 0x0422
    {"en", MAKELANGID(LANG_ENGLISH,   SUBLANG_ENGLISH_US)},         // 0x0409
};

class SwitcherWin final : public ISwitcher {
public:
    bool switch_layout(std::string_view locale) override {
        const auto it = LOCALE_TO_LANGID.find(std::string(locale));
        if (it == LOCALE_TO_LANGID.end()) return false;

        const LANGID langid = it->second;
        wchar_t klid[9] = {};
        swprintf_s(klid, 9, L"%08X", static_cast<unsigned>(langid));
        const HKL target = LoadKeyboardLayoutW(klid, KLF_ACTIVATE);
        if (!target) return false;

        const HWND fg = GetForegroundWindow();
        PostMessageW(fg, WM_INPUTLANGCHANGEREQUEST, 0,
                     reinterpret_cast<LPARAM>(target));
        return true;
    }

    void retype(std::size_t char_count, std::string_view text) override {
        const std::size_t total_inputs = char_count + text.size() * 2;
        if (total_inputs == 0) return;

        std::vector<INPUT> inputs;
        inputs.reserve(total_inputs);

        // Backspace × char_count
        for (std::size_t i = 0; i < char_count; ++i) {
            INPUT in{};
            in.type = INPUT_KEYBOARD;
            in.ki.wVk = VK_BACK;
            inputs.push_back(in);
            in.ki.dwFlags = KEYEVENTF_KEYUP;
            inputs.push_back(in);
        }

        // Unicode keydown + keyup for each character in text
        // text is UTF-8; decode to UTF-16 for SendInput
        const int wlen = MultiByteToWideChar(
            CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (wlen > 0) {
            std::wstring wtext(wlen, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, text.data(),
                                static_cast<int>(text.size()),
                                wtext.data(), wlen);
            for (wchar_t wch : wtext) {
                INPUT in{};
                in.type = INPUT_KEYBOARD;
                in.ki.wVk = 0;
                in.ki.wScan = wch;
                in.ki.dwFlags = KEYEVENTF_UNICODE;
                inputs.push_back(in);
                in.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
                inputs.push_back(in);
            }
        }

        SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    }

};

} // namespace

std::unique_ptr<ISwitcher> ISwitcher::create() {
    return std::make_unique<SwitcherWin>();
}

} // namespace clavi
