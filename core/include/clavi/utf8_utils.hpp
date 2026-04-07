#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace clavi::utf8 {

// Decode a single UTF-8 sequence; advances p, returns 0xFFFD on error.
inline uint32_t decode(const char*& p, const char* end) noexcept {
    if (p >= end) return 0;
    const auto b0 = static_cast<uint8_t>(*p++);
    if (b0 < 0x80) return b0;
    if (b0 < 0xC0) return 0xFFFD;
    int extra = 0;
    uint32_t cp = 0;
    if      (b0 < 0xE0) { extra = 1; cp = b0 & 0x1F; }
    else if (b0 < 0xF0) { extra = 2; cp = b0 & 0x0F; }
    else if (b0 < 0xF8) { extra = 3; cp = b0 & 0x07; }
    else return 0xFFFD;
    for (int i = 0; i < extra; ++i) {
        if (p >= end) return 0xFFFD;
        const auto b = static_cast<uint8_t>(*p++);
        if ((b & 0xC0) != 0x80) return 0xFFFD;
        cp = (cp << 6) | (b & 0x3F);
    }
    return cp;
}

// Encode a codepoint to UTF-8, appending to out.
inline void encode(uint32_t cp, std::string& out) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

// Fold a codepoint to lowercase (ASCII + Ukrainian Cyrillic).
inline uint32_t to_lower_cp(uint32_t cp) noexcept {
    // ASCII
    if (cp < 0x80) {
        if (cp >= 'A' && cp <= 'Z') return cp + 32;
        return cp;
    }
    // Cyrillic А-Я → а-я (U+0410..U+042F → U+0430..U+044F)
    if (cp >= 0x0410 && cp <= 0x042F) return cp + 0x20;
    // Ukrainian special uppercase
    if (cp == 0x0404) return 0x0454; // Є → є
    if (cp == 0x0406) return 0x0456; // І → і
    if (cp == 0x0407) return 0x0457; // Ї → ї
    if (cp == 0x0490) return 0x0491; // Ґ → ґ
    return cp;
}

// Return the lowercased UTF-8 representation of s.
inline std::string to_lower(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    const char* p = s.data();
    const char* end = p + s.size();
    while (p < end) {
        const uint32_t cp = decode(p, end);
        if (cp == 0xFFFD) { out += '\xEF'; out += '\xBF'; out += '\xBD'; continue; }
        encode(to_lower_cp(cp), out);
    }
    return out;
}

// Count codepoints in a UTF-8 string.
inline std::size_t count(std::string_view s) noexcept {
    std::size_t n = 0;
    const char* p = s.data();
    const char* end = p + s.size();
    while (p < end) { decode(p, end); ++n; }
    return n;
}

} // namespace clavi::utf8
