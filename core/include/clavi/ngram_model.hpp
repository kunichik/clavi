#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace clavi {

// Character n-gram language model loaded from ngram.bin.
// Binary format:
//   4-byte magic "NGRM"
//   4-byte LE entry count
//   1-byte ngram size N (3, 4, or 5)
//   Then count entries, each: [N bytes UTF-8 ngram] [4-byte float LE log-prob]
//   Entries sorted alphabetically by ngram for binary search.
//
// Scoring: sum of log-probabilities of all N-grams in the input text,
// normalized by the number of N-grams extracted.
class NgramModel {
public:
    NgramModel() = default;

    // Load from binary file. Returns false on failure.
    [[nodiscard]] bool load(std::string_view path);

    // Score a UTF-8 text. Returns average log-probability of its character
    // n-grams, or std::nullopt if the text is too short to extract any n-gram.
    // Higher (less negative) = more likely to be this language.
    [[nodiscard]] std::optional<double> score(std::string_view text) const;

    [[nodiscard]] bool loaded() const noexcept { return !entries_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    [[nodiscard]] uint8_t ngram_size() const noexcept { return ngram_n_; }

private:
    struct Entry {
        std::string ngram; // exactly ngram_n_ bytes
        float log_prob{0.0f};
    };

    std::vector<Entry> entries_;
    uint8_t ngram_n_{3};

    // Default log-probability for unseen n-grams (heavy penalty).
    static constexpr float UNSEEN_LOG_PROB = -10.0f;

    // Binary search for an n-gram. Returns its log_prob or UNSEEN_LOG_PROB.
    [[nodiscard]] float lookup(std::string_view ngram) const noexcept;
};

} // namespace clavi
