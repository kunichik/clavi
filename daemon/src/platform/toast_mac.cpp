#include "clavi/platform/toast.hpp"

#include <cstdlib>
#include <string>

namespace clavi {

namespace {

class ToastMac final : public IToast {
public:
    void show(std::string_view title, std::string_view body,
              int /*timeout_ms*/) override {
        // macOS notifications via osascript (works without codesigning in dev)
        auto escape = [](std::string_view s) {
            std::string out;
            out.reserve(s.size() + 4);
            for (char c : s) {
                if (c == '"')  out += "\\\"";
                else if (c == '\\') out += "\\\\";
                else out += c;
            }
            return out;
        };

        const std::string cmd =
            std::string("osascript -e 'display notification \"") +
            escape(body) + "\" with title \"" + escape(title) +
            "\"' 2>/dev/null";
        std::system(cmd.c_str());
    }
};

} // namespace

std::unique_ptr<IToast> IToast::create() {
    return std::make_unique<ToastMac>();
}

} // namespace clavi
