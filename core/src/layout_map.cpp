#include "clavi/layout_map.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

namespace clavi {

namespace {

constexpr uint8_t MAGIC[4] = {'K', 'M', 'A', 'P'};

// Decode a single UTF-8 sequence; returns codepoint and advances *p.
// Returns 0xFFFD on invalid input.
uint32_t utf8_decode(const char*& p, const char* end) noexcept {
    if (p >= end) return 0;
    const auto b0 = static_cast<uint8_t>(*p);
    ++p;
    if (b0 < 0x80) return b0;
    if (b0 < 0xC0) return 0xFFFD; // continuation byte without leader
    int extra = 0;
    uint32_t cp = 0;
    if (b0 < 0xE0) { extra = 1; cp = b0 & 0x1F; }
    else if (b0 < 0xF0) { extra = 2; cp = b0 & 0x0F; }
    else if (b0 < 0xF8) { extra = 3; cp = b0 & 0x07; }
    else return 0xFFFD;
    for (int i = 0; i < extra; ++i) {
        if (p >= end) return 0xFFFD;
        const auto b = static_cast<uint8_t>(*p);
        if ((b & 0xC0) != 0x80) return 0xFFFD;
        cp = (cp << 6) | (b & 0x3F);
        ++p;
    }
    return cp;
}

// Encode a codepoint to UTF-8, appending to out.
void utf8_encode(uint32_t cp, std::string& out) {
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

} // namespace

uint32_t LayoutMap::read_u32_le(const uint8_t* p) noexcept {
    uint32_t v{};
    std::memcpy(&v, p, 4);
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    v = __builtin_bswap32(v);
#endif
    return v;
}

bool LayoutMap::load(std::string_view path) {
    std::ifstream f(std::string(path), std::ios::binary | std::ios::ate);
    if (!f.is_open()) return false;

    const auto size = static_cast<std::size_t>(f.tellg());
    f.seekg(0);

    if (size < 8) return false;

    std::vector<uint8_t> buf(size);
    f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(size));
    if (!f) return false;

    if (std::memcmp(buf.data(), MAGIC, 4) != 0) return false;

    const uint32_t count = read_u32_le(buf.data() + 4);
    const std::size_t expected = 8 + static_cast<std::size_t>(count) * 8;
    if (size < expected) return false;

    pairs_.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* entry = buf.data() + 8 + i * 8;
        pairs_[i].source = read_u32_le(entry);
        pairs_[i].target = read_u32_le(entry + 4);
    }

    // Entries are pre-sorted by source codepoint for binary search
    return true;
}

std::optional<uint32_t> LayoutMap::remap_codepoint(uint32_t cp) const noexcept {
    // Binary search by source codepoint
    auto it = std::lower_bound(pairs_.begin(), pairs_.end(), cp,
        [](const KeyPair& kp, uint32_t val) { return kp.source < val; });
    if (it != pairs_.end() && it->source == cp) {
        return it->target;
    }
    return std::nullopt;
}

std::string LayoutMap::remap(std::string_view text) const {
    std::string result;
    result.reserve(text.size());
    const char* p = text.data();
    const char* end = p + text.size();
    while (p < end) {
        const uint32_t cp = utf8_decode(p, end);
        if (cp == 0xFFFD) {
            result += '\xEF';
            result += '\xBF';
            result += '\xBD';
            continue;
        }
        const auto mapped = remap_codepoint(cp);
        utf8_encode(mapped.value_or(cp), result);
    }
    return result;
}

} // namespace clavi
