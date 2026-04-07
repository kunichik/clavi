#include "clavi/ngram_model.hpp"
#include "clavi/utf8_utils.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>

namespace clavi {

// ── Binary loader ────────────────────────────────────────────────────────────

bool NgramModel::load(std::string_view path) {
    std::ifstream f(std::string(path), std::ios::binary);
    if (!f) return false;

    // Magic: "NGRM"
    char magic[4]{};
    f.read(magic, 4);
    if (std::memcmp(magic, "NGRM", 4) != 0) return false;

    // Entry count (LE u32)
    uint32_t count = 0;
    f.read(reinterpret_cast<char*>(&count), 4);

    // N-gram size (1 byte)
    uint8_t n = 0;
    f.read(reinterpret_cast<char*>(&n), 1);
    if (n < 2 || n > 8) return false;

    ngram_n_ = n;
    entries_.clear();
    entries_.reserve(count);

    std::string buf(n, '\0');
    for (uint32_t i = 0; i < count; ++i) {
        f.read(buf.data(), n);
        float lp = 0.0f;
        f.read(reinterpret_cast<char*>(&lp), 4);
        if (!f) return false;
        entries_.push_back({buf, lp});
    }

    // Verify sorted (binary search requirement)
    for (std::size_t i = 1; i < entries_.size(); ++i) {
        if (entries_[i].ngram < entries_[i - 1].ngram) return false;
    }

    return true;
}

// ── Lookup ───────────────────────────────────────────────────────────────────

float NgramModel::lookup(std::string_view ngram) const noexcept {
    // Binary search over sorted entries
    std::size_t lo = 0;
    std::size_t hi = entries_.size();
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        const int cmp = entries_[mid].ngram.compare(
            0, std::string::npos, ngram.data(), ngram.size());
        if (cmp < 0) {
            lo = mid + 1;
        } else if (cmp > 0) {
            hi = mid;
        } else {
            return entries_[mid].log_prob;
        }
    }
    return UNSEEN_LOG_PROB;
}

// ── Scoring ──────────────────────────────────────────────────────────────────

std::optional<double> NgramModel::score(std::string_view text) const {
    if (entries_.empty()) return std::nullopt;

    // Lowercase for case-insensitive scoring
    const std::string lower = utf8::to_lower(text);

    if (lower.size() < ngram_n_) return std::nullopt;

    // Byte-level sliding window (matches binary format: ngram_n_ bytes each)
    double total = 0.0;
    std::size_t ngram_count = 0;

    for (std::size_t i = 0; i + ngram_n_ <= lower.size(); ++i) {
        const std::string_view ng(lower.data() + i, ngram_n_);
        total += lookup(ng);
        ++ngram_count;
    }

    if (ngram_count == 0) return std::nullopt;
    return total / static_cast<double>(ngram_count);
}

} // namespace clavi
