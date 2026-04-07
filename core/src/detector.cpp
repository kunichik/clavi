#include "clavi/detector.hpp"
#include "clavi/pack_loader.hpp"
#include "clavi/utf8_utils.hpp"

#include <array>
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace clavi {

namespace {

// Words that exist in both UK and EN and should never trigger a switch
constexpr std::array<std::string_view, 12> AMBIGUOUS_SKIP_LIST = {
    "a", "i", "o", "I", "in", "on", "at", "to", "do", "no", "ok", "OK"
};

} // namespace

bool Detector::is_ambiguous_token(std::string_view word) noexcept {
    for (auto tok : AMBIGUOUS_SKIP_LIST) {
        if (word == tok) return true;
    }
    return false;
}

std::string Detector::to_lowercase(std::string_view word) {
    return utf8::to_lower(word);
}

bool Detector::load_pack(std::string_view pack_dir) {
    namespace fs = std::filesystem;
    const fs::path dir(pack_dir);

    // Read locale from pack.toml (simple scan — full TOML parse in pack_loader.cpp)
    const fs::path pack_toml = dir / "pack.toml";
    std::string locale;
    {
        std::ifstream f(pack_toml);
        if (!f.is_open()) return false;
        std::string line;
        while (std::getline(f, line)) {
            if (line.find("locale") != std::string::npos &&
                line.find('=') != std::string::npos) {
                const auto q1 = line.find('"');
                const auto q2 = line.rfind('"');
                if (q1 != std::string::npos && q2 != q1) {
                    locale = line.substr(q1 + 1, q2 - q1 - 1);
                    break;
                }
            }
        }
    }
    if (locale.empty()) return false;
    if (!PackLoader::is_allowed(locale)) return false;

    LoadedPack pack;
    pack.locale = locale;

    const fs::path dict_path = dir / "dictionary.bin";
    if (!pack.dictionary.load(dict_path.string())) return false;

    const fs::path kmap_path = dir / "keyboard_map.bin";
    if (!pack.to_this.load(kmap_path.string())) return false;

    // Layer 2: n-gram model (optional — missing file is not an error)
    const fs::path ngram_path = dir / "ngram.bin";
    (void)pack.ngram.load(ngram_path.string()); // optional, missing is OK

    // Translit rules (optional — missing file is not an error)
    const fs::path translit_path = dir / "translit.toml";
    (void)pack.translit.load(translit_path.string()); // optional, missing is OK

    packs_.push_back(std::move(pack));
    return true;
}

DetectionResult Detector::analyze(std::string_view typed_word) const {
    // UX Rule 3: never switch on short words
    if (utf8::count(typed_word) <= MIN_WORD_LENGTH - 1) {
        return DetectionResult{Action::NoAction, {}, {}};
    }

    // Ambiguous token skip-list
    if (is_ambiguous_token(typed_word)) {
        return DetectionResult{Action::NoAction, {}, {}};
    }

    const std::string lower_typed = to_lowercase(typed_word);

    // Check which packs the typed word already matches
    std::vector<std::string> typed_matches;
    for (const auto& pack : packs_) {
        if (pack.dictionary.contains(lower_typed)) {
            typed_matches.push_back(pack.locale);
        }
    }

    // If the typed word matches exactly one pack → it's correct, no action
    if (typed_matches.size() == 1) {
        return DetectionResult{Action::NoAction, {}, {}};
    }

    // If it matches all packs → ambiguous, no action
    if (typed_matches.size() == packs_.size() && !packs_.empty()) {
        return DetectionResult{Action::NoAction, {}, {}};
    }

    // Remap the typed word through each pack's layout map and see if the remapped
    // version is found in that pack's dictionary
    struct Candidate {
        std::string locale;
        std::string remapped;
    };
    std::vector<Candidate> candidates;

    for (const auto& candidate_pack : packs_) {
        const std::string remapped = candidate_pack.to_this.remap(typed_word);
        if (remapped == typed_word) continue; // no change

        const std::string lower_remapped = to_lowercase(remapped);
        if (candidate_pack.dictionary.contains(lower_remapped)) {
            if (typed_matches.empty()) {
                candidates.push_back({candidate_pack.locale, remapped});
            }
        }
    }

    // Layer 1: single unambiguous candidate → switch
    if (candidates.size() == 1) {
        return DetectionResult{
            Action::SwitchAndRetype,
            candidates[0].locale,
            candidates[0].remapped
        };
    }

    // Layer 2: multiple candidates — use n-gram scoring as tiebreaker
    if (candidates.size() > 1) {
        double best_score = -1e9;
        std::size_t best_idx = 0;
        bool have_scores = false;

        for (std::size_t i = 0; i < candidates.size(); ++i) {
            for (const auto& pack : packs_) {
                if (pack.locale == candidates[i].locale && pack.ngram.loaded()) {
                    const auto s = pack.ngram.score(candidates[i].remapped);
                    if (s && *s > best_score) {
                        best_score = *s;
                        best_idx = i;
                        have_scores = true;
                    }
                    break;
                }
            }
        }

        if (have_scores) {
            return DetectionResult{
                Action::SwitchAndRetype,
                candidates[best_idx].locale,
                candidates[best_idx].remapped
            };
        }
        // If no n-gram models loaded, fall through to NoAction
    }

    // Layer 2 standalone: typed word not in any dict AND no remap candidate,
    // but n-gram scoring might still identify the language of the typed text
    // to suggest the current layout is wrong. (Future enhancement)

    return DetectionResult{Action::NoAction, {}, {}};
}

std::string Detector::score_ngrams(std::string_view text,
                                    double threshold) const {
    double best_score = -1e9;
    std::string best_locale;

    for (const auto& pack : packs_) {
        if (!pack.ngram.loaded()) continue;
        const auto s = pack.ngram.score(text);
        if (s && *s > best_score) {
            best_score = *s;
            best_locale = pack.locale;
        }
    }

    // If best score is below threshold, return empty (inconclusive)
    if (best_score < threshold && threshold != 0.0) return {};
    return best_locale;
}

std::string Detector::transliterate(std::string_view latin_text,
                                     std::string_view locale) const {
    for (const auto& pack : packs_) {
        if (pack.locale == locale && !pack.translit.empty()) {
            return pack.translit.transliterate(latin_text);
        }
    }
    return {};
}

} // namespace clavi
