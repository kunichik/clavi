#pragma once

#include "dictionary.hpp"
#include "layout_map.hpp"
#include "ngram_model.hpp"
#include "translit.hpp"

#include <string>
#include <string_view>
#include <unordered_set>

namespace clavi {

enum class Action {
    NoAction,
    SwitchAndRetype,
};

struct DetectionResult {
    Action action{Action::NoAction};
    std::string target_locale{};
    std::string corrected_text{};
};

struct LoadedPack {
    std::string locale;
    Dictionary dictionary;
    LayoutMap to_this;
    NgramModel ngram;   // Layer 2 (optional — empty if ngram.bin absent)
    Translit translit;  // Translit mode (optional — empty if translit.toml absent)
};

class Detector {
public:
    Detector() = default;

    [[nodiscard]] bool load_pack(std::string_view pack_dir);

    // Set exclusion words (from exclusions.toml [words].skip).
    // These words will never trigger a switch.
    void set_skip_words(std::vector<std::string> words) noexcept;

    [[nodiscard]] DetectionResult analyze(std::string_view typed_word) const;

    [[nodiscard]] std::size_t pack_count() const noexcept { return packs_.size(); }

    // Translit: convert Latin text to target locale script.
    // Returns empty string if no translit rules loaded for that locale.
    [[nodiscard]] std::string transliterate(std::string_view latin_text,
                                            std::string_view locale) const;

private:
    std::vector<LoadedPack> packs_;
    std::unordered_set<std::string> skip_words_;

    static constexpr std::size_t MIN_WORD_LENGTH = 3;

    [[nodiscard]] static bool is_ambiguous_token(std::string_view word) noexcept;
    [[nodiscard]] static std::string to_lowercase(std::string_view word);

    // Layer 2: score text against all packs' n-gram models.
    // Returns locale of best-scoring pack, or empty if inconclusive.
    [[nodiscard]] std::string score_ngrams(std::string_view text,
                                           double threshold) const;
};

} // namespace clavi
