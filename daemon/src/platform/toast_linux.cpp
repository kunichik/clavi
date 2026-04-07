#include "clavi/platform/toast.hpp"

#include <cstdlib>
#include <string>

namespace clavi {

namespace {

class ToastLinux final : public IToast {
public:
    void show(std::string_view title, std::string_view body,
              int timeout_ms) override {
        // notify-send is available on most distros with libnotify-bin
        // Escape single quotes in both strings
        auto escape = [](std::string_view s) {
            std::string out;
            out.reserve(s.size() + 4);
            for (char c : s) {
                if (c == '\'') out += "'\\''";
                else out += c;
            }
            return out;
        };

        const int timeout_sec = timeout_ms > 0 ? timeout_ms : 2000;
        const std::string cmd =
            std::string("notify-send -t ") + std::to_string(timeout_sec) +
            " -a Clavi '" + escape(title) + "' '" + escape(body) +
            "' 2>/dev/null";
        std::system(cmd.c_str());
    }
};

} // namespace

std::unique_ptr<IToast> IToast::create() {
    return std::make_unique<ToastLinux>();
}

} // namespace clavi
