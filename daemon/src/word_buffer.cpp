#include "clavi/word_buffer.hpp"
#include "clavi/utf8_utils.hpp"

namespace clavi {

WordBuffer::WordBuffer(std::size_t max_word_bytes) noexcept
    : max_word_bytes_(max_word_bytes) {}

bool WordBuffer::is_word_boundary(uint32_t cp) noexcept {
    // ASCII boundaries: space, tab, CR, LF, common punctuation
    if (cp <= 0x7F) {
        switch (cp) {
            case ' ': case '\t': case '\r': case '\n':
            case '.': case ',': case ';': case ':':
            case '!': case '?': case '"': case '\'':
            case '(': case ')': case '[': case ']':
            case '{': case '}': case '<': case '>':
            case '/': case '\\': case '|':
            case '\0':
                return true;
            default:
                return false;
        }
    }
    // Non-breaking space U+00A0
    if (cp == 0x00A0) return true;
    // Ukrainian guillemets used as quotes
    if (cp == 0x00AB || cp == 0x00BB) return true;
    return false;
}

void WordBuffer::feed_codepoint(uint32_t cp) {
    if (is_word_boundary(cp)) {
        flush();
        return;
    }
    // Overflow protection: if word is too long, flush and start fresh
    if (buf_.size() + 4 > max_word_bytes_) {
        buf_.clear();
        return;
    }
    utf8::encode(cp, buf_);
}

void WordBuffer::feed_backspace() {
    if (buf_.empty()) return;
    // Remove last UTF-8 codepoint: walk back from end skipping continuation bytes
    auto it = buf_.end();
    --it;
    while (it != buf_.begin() && (static_cast<uint8_t>(*it) & 0xC0) == 0x80) {
        --it;
    }
    buf_.erase(it, buf_.end());
}

void WordBuffer::feed_clear() {
    buf_.clear();
}

void WordBuffer::flush() {
    if (!buf_.empty() && on_commit_) {
        on_commit_(buf_);
    }
    buf_.clear();
}

} // namespace clavi
