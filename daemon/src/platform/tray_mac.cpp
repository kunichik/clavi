#include "clavi/platform/tray.hpp"

// macOS tray: minimal Objective-C++ would be needed for NSStatusItem.
// For now, provide a no-op stub — toast notifications still work via osascript.
// Full NSStatusBar integration is planned for v1.1+.

namespace clavi {

namespace {

class TrayMac final : public ITray {
public:
    void init(Callbacks /*cbs*/) override {}
    void set_enabled(bool /*enabled*/) override {}
    void shutdown() override {}
};

} // namespace

std::unique_ptr<ITray> ITray::create() {
    return std::make_unique<TrayMac>();
}

} // namespace clavi
