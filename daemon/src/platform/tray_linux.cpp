#include "clavi/platform/tray.hpp"

// Linux tray: fragmented ecosystem (libappindicator, GTK StatusIcon deprecated,
// SNI protocol varies by DE). Provide a no-op stub for now.
// Toast notifications still work via notify-send.
// Full tray integration planned for v1.1+ (target: libappindicator3).

namespace clavi {

namespace {

class TrayLinux final : public ITray {
public:
    void init(Callbacks /*cbs*/) override {}
    void set_enabled(bool /*enabled*/) override {}
    void shutdown() override {}
};

} // namespace

std::unique_ptr<ITray> ITray::create() {
    return std::make_unique<TrayLinux>();
}

} // namespace clavi
