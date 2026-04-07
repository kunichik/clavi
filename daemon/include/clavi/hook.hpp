#pragma once

#include <atomic>
#include <functional>
#include <string>

namespace clavi {

// Called on every committed word (space/punctuation/enter boundary)
using WordCommitCallback = std::function<void(const std::string& word)>;

// Called on undo hotkey press
using UndoCallback = std::function<void()>;

// Called on toggle hotkey press
using ToggleCallback = std::function<void()>;

struct HookCallbacks {
    WordCommitCallback on_word_commit;
    UndoCallback on_undo;
    ToggleCallback on_toggle;
};

class Hook {
public:
    Hook() = default;
    ~Hook();

    Hook(const Hook&) = delete;
    Hook& operator=(const Hook&) = delete;

    // Start the keyboard hook (blocks calling thread until stop() is called)
    void run(HookCallbacks callbacks);

    // Signal the hook to stop (safe to call from any thread)
    void stop() noexcept;

    [[nodiscard]] bool is_running() const noexcept { return running_.load(); }

private:
    std::atomic<bool> running_{false};
};

} // namespace clavi
