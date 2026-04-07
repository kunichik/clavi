#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace clavi {

// Accumulates keystrokes and emits completed words when a word boundary is hit.
// Word boundaries: space, tab, newline, most punctuation, Backspace erases last char.
// Thread-compatible but NOT thread-safe (call from a single hook thread).
class WordBuffer {
public:
    explicit WordBuffer(std::size_t max_word_bytes = 256) noexcept;

    using CommitCallback = std::function<void(const std::string& word)>;

    void set_commit_callback(CommitCallback cb) { on_commit_ = std::move(cb); }

    // Feed a single Unicode codepoint from the keyboard hook.
    void feed_codepoint(uint32_t cp);

    // Feed a special key event.
    void feed_backspace();
    void feed_clear();   // Escape / Ctrl+A → forget current word

    // Force-emit current word if non-empty (e.g. focus lost).
    void flush();

    void reset() noexcept { buf_.clear(); }

    [[nodiscard]] const std::string& current() const noexcept { return buf_; }
    [[nodiscard]] bool empty() const noexcept { return buf_.empty(); }

private:
    std::string buf_;
    std::size_t max_word_bytes_;
    CommitCallback on_commit_;

    static bool is_word_boundary(uint32_t cp) noexcept;
};

} // namespace clavi
