#include "clavi/layout_map.hpp"
#include "clavi/utf8_utils.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <vector>

namespace clavi {

namespace {

constexpr uint8_t MAGIC[4] = {'K', 'M', 'A', 'P'};

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
        const uint32_t cp = utf8::decode(p, end);
        if (cp == 0xFFFD) {
            result += '\xEF';
            result += '\xBF';
            result += '\xBD';
            continue;
        }
        const auto mapped = remap_codepoint(cp);
        utf8::encode(mapped.value_or(cp), result);
    }
    return result;
}

} // namespace clavi
