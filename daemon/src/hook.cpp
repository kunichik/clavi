#include "clavi/hook.hpp"
#include "clavi/word_buffer.hpp"

#include <uiohook.h>
#include <atomic>
#include <cstdio>
#include <cstdarg>

namespace clavi {

// ── Module-level state (used from libuiohook C callbacks) ──────────────────

static std::atomic<bool> g_running{false};
static HookCallbacks     g_callbacks;
static WordBuffer*       g_word_buf = nullptr;

static bool g_ctrl  = false;
static bool g_shift = false;
static bool g_alt   = false;

// ── libuiohook event dispatcher ─────────────────────────────────────────────

static void dispatch_event(uiohook_event* const ev) {
    if (!ev) return;

    switch (ev->type) {

        case EVENT_KEY_PRESSED: {
            const uint16_t kc = ev->data.keyboard.keycode;

            // Track modifiers
            if (kc == VC_CONTROL_L || kc == VC_CONTROL_R) { g_ctrl  = true; break; }
            if (kc == VC_SHIFT_L   || kc == VC_SHIFT_R)   { g_shift = true; break; }
            if (kc == VC_ALT_L     || kc == VC_ALT_R)     { g_alt   = true; break; }

            // Ctrl+Z → undo (without Shift)
            if (g_ctrl && !g_shift && !g_alt && kc == VC_Z) {
                if (g_word_buf) g_word_buf->feed_clear();
                if (g_callbacks.on_undo) g_callbacks.on_undo();
                break;
            }

            // Ctrl+Shift+Space → toggle
            if (g_ctrl && g_shift && !g_alt && kc == VC_SPACE) {
                if (g_word_buf) g_word_buf->feed_clear();
                if (g_callbacks.on_toggle) g_callbacks.on_toggle();
                break;
            }

            // Backspace → erase last codepoint in buffer
            if (kc == VC_BACKSPACE) {
                if (g_word_buf) g_word_buf->feed_backspace();
                break;
            }

            // Enter / Return → flush current word
            if (kc == VC_ENTER || kc == VC_KP_ENTER) {
                if (g_word_buf) g_word_buf->flush();
                break;
            }

            // Escape → discard current word
            if (kc == VC_ESCAPE) {
                if (g_word_buf) g_word_buf->feed_clear();
                break;
            }

            // Any modified keystroke (Ctrl+X etc.) clears the buffer
            if (g_ctrl || g_alt) {
                if (g_word_buf) g_word_buf->feed_clear();
            }
            break;
        }

        case EVENT_KEY_RELEASED: {
            const uint16_t kc = ev->data.keyboard.keycode;
            if (kc == VC_CONTROL_L || kc == VC_CONTROL_R) g_ctrl  = false;
            if (kc == VC_SHIFT_L   || kc == VC_SHIFT_R)   g_shift = false;
            if (kc == VC_ALT_L     || kc == VC_ALT_R)     g_alt   = false;
            break;
        }

        case EVENT_KEY_TYPED: {
            // EVENT_KEY_TYPED delivers the Unicode BMP character.
            const uint16_t ch = ev->data.keyboard.keychar;
            if (ch == CHAR_UNDEFINED || g_ctrl || g_alt) break;

            if (g_word_buf) g_word_buf->feed_codepoint(static_cast<uint32_t>(ch));
            break;
        }

        default:
            break;
    }
}

static bool logger_callback(unsigned int level, const char* fmt, ...) {
    if (level == LOG_LEVEL_ERROR) {
        va_list args;
        va_start(args, fmt);
        std::vfprintf(stderr, fmt, args);
        va_end(args);
        std::fputc('\n', stderr);
    }
    return level != LOG_LEVEL_ERROR;
}

// ── Hook ────────────────────────────────────────────────────────────────────

Hook::~Hook() { stop(); }

void Hook::run(HookCallbacks callbacks) {
    g_callbacks = std::move(callbacks);

    WordBuffer wbuf;
    wbuf.set_commit_callback([](const std::string& word) {
        if (g_callbacks.on_word_commit) g_callbacks.on_word_commit(word);
    });
    g_word_buf  = &wbuf;
    g_running.store(true);
    running_ = true;

    hook_set_logger_proc(&logger_callback);
    hook_set_dispatch_proc(&dispatch_event);

    hook_run(); // blocks until hook_stop() is called

    g_word_buf = nullptr;
    running_   = false;
    g_running.store(false);
}

void Hook::stop() noexcept {
    if (g_running.load()) hook_stop();
}

} // namespace clavi
