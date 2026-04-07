#include "clavi/dictionary.hpp"
#include "clavi/utf8_utils.hpp"

#include <cstring>
#include <fstream>
#include <vector>

#define XXH_INLINE_ALL
#include <xxhash.h>

namespace clavi {

namespace {

constexpr uint8_t MAGIC[4] = {'C', 'L', 'A', 'V'};
constexpr uint64_t EMPTY_SLOT = 0x0000000000000000ULL;

} // namespace

uint32_t Dictionary::read_u32_le(const uint8_t* p) noexcept {
    uint32_t v{};
    std::memcpy(&v, p, 4);
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    v = __builtin_bswap32(v);
#endif
    return v;
}

uint64_t Dictionary::read_u64_le(const uint8_t* p) noexcept {
    uint64_t v{};
    std::memcpy(&v, p, 8);
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    v = __builtin_bswap64(v);
#endif
    return v;
}

uint64_t Dictionary::hash_word(std::string_view word) noexcept {
    const uint64_t h = XXH3_64bits(word.data(), word.size());
    // Reserve 0 as empty-slot sentinel; remap to 1 if hash collision
    return h == 0 ? 1 : h;
}

bool Dictionary::load(std::string_view path) {
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
    // Table size follows: next power of 2 >= ceil(count / 0.7)
    const std::size_t table_size = (size - 8) / 8;
    if (table_size == 0) return false;

    table_.resize(table_size);
    for (std::size_t i = 0; i < table_size; ++i) {
        table_[i] = read_u64_le(buf.data() + 8 + i * 8);
    }

    (void)count;
    return true;
}

bool Dictionary::contains(std::string_view word) const noexcept {
    if (table_.empty()) return false;

    const std::string lower = utf8::to_lower(word);
    const uint64_t h = hash_word(lower);
    const std::size_t mask = table_.size() - 1;
    std::size_t idx = static_cast<std::size_t>(h) & mask;

    for (std::size_t probe = 0; probe < table_.size(); ++probe) {
        const uint64_t slot = table_[idx];
        if (slot == EMPTY_SLOT) return false;
        if (slot == h) return true;
        idx = (idx + 1) & mask;
    }
    return false;
}

} // namespace clavi
