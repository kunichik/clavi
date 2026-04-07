#pragma once

#include <functional>
#include <memory>

namespace clavi {

// Abstract system-tray icon with right-click context menu.
// Menu items: "Enable / Disable", separator, "Quit".
class ITray {
public:
    virtual ~ITray() = default;

    struct Callbacks {
        std::function<void()> on_toggle; // enable ↔ disable
        std::function<void()> on_quit;   // exit daemon
    };

    // Initialise tray icon and start its internal message pump (may spawn a
    // thread). Must be called once, before the hook starts.
    virtual void init(Callbacks cbs) = 0;

    // Update the visual state (tooltip and/or icon) to reflect enabled/paused.
    virtual void set_enabled(bool enabled) = 0;

    // Tear down the tray icon (called automatically by destructor too).
    virtual void shutdown() = 0;

    // Factory: returns the correct implementation for the current platform.
    [[nodiscard]] static std::unique_ptr<ITray> create();
};

} // namespace clavi
