#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace clavi {

// Abstract interface for system toast / desktop notification.
class IToast {
public:
    virtual ~IToast() = default;

    // Show a brief non-blocking notification.
    // title: short title (e.g. "Clavi")
    // body: body text (e.g. "привіт ← ghbdsn")
    // timeout_ms: auto-dismiss after this many milliseconds (0 = platform default)
    virtual void show(std::string_view title,
                      std::string_view body,
                      int timeout_ms = 2000) = 0;

    // Factory: returns the correct implementation for the current platform.
    [[nodiscard]] static std::unique_ptr<IToast> create();
};

} // namespace clavi
