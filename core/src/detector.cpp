#include "clavi/detector.hpp"
#include "clavi/pack_loader.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

namespace clavi {

namespace {

// Words that exist in both UK and EN and should never trigger a switch
constexpr std::array<std::string_view, 12> AMBIGUOUS_SKIP_LIST = {
    "a", "i", "o", "I", "in", "on", "at", "to", "do", "no", "ok", "OK"
};

// Encode a Unicode codepoint to lowercase UTF-8 byte sequence
// For ASCII we can use tolower; for Cyrillic we handle the ranges directly
std::string codepoint_to_lower_utf8(uint32_t cp) {
    std::string out;
    // Cyrillic uppercase А-Я (0x0410-0x042F) → а-я (0x0430-0x044F)
    if (cp >= 0x0410 && cp <= 0x042F) cp += 0x20;
    // Ukrainian Є (0x0404) → є (0x0454)
    if (cp == 0x0404) cp = 0x0454;
    // Ukrainian І (0x0406) → і (0x0456)
    if (cp == 0x0406) cp = 0x0456;
    // Ukrainian Ї (0x0407) → ї (0x0457)
    if (cp == 0x0407) cp = 0x0457;
    // Ukrainian Ґ (0x0490) → ґ (0x0491)
    if (cp == 0x0490) cp = 0x0491;
    // ASCII
    if (cp < 0x80) {
        out += static_cast<char>(std::tolower(static_cast<int>(cp)));
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return out;
}

uint32_t utf8_decode_one(const char*& p, const char* end) noexcept {
    if (p >= end) return 0;
    const auto b0 = static_cast<uint8_t>(*p++);
    if (b0 < 0x80) return b0;
    if (b0 < 0xC0) return 0xFFFD;
    int extra = 0;
    uint32_t cp = 0;
    if (b0 < 0xE0) { extra = 1; cp = b0 & 0x1F; }
    else if (b0 < 0xF0) { extra = 2; cp = b0 & 0x0F; }
    else { extra = 3; cp = b0 & 0x07; }
    for (int i = 0; i < extra; ++i) {
        if (p >= end) return 0xFFFD;
        const auto b = static_cast<uint8_t>(*p++);
        if ((b & 0xC0) != 0x80) return 0xFFFD;
        cp = (cp << 6) | (b & 0x3F);
    }
    return cp;
}

std::size_t utf8_codepoint_count(std::string_view s) noexcept {
    std::size_t count = 0;
    const char* p = s.data();
    const char* end = p + s.size();
    while (p < end) {
        utf8_decode_one(p, end);
        ++count;
    }
    return count;
}

} // namespace

bool Detector::is_ambiguous_token(std::string_view word) noexcept {
    for (auto tok : AMBIGUOUS_SKIP_LIST) {
        if (word == tok) return true;
    }
    return false;
}

std::string Detector::to_lowercase(std::string_view word) {
    std::string out;
    out.reserve(word.size());
    const char* p = word.data();
    const char* end = p + word.size();
    while (p < end) {
        const uint32_t cp = utf8_decode_one(p, end);
        out += codepoint_to_lower_utf8(cp);
    }
    return out;
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

    packs_.push_back(std::move(pack));
    return true;
}

DetectionResult Detector::analyze(std::string_view typed_word) const {
    // UX Rule 3: never switch on short words
    if (utf8_codepoint_count(typed_word) <= MIN_WORD_LENGTH - 1) {
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
    for (const auto& candidate_pack : packs_) {
        const std::string remapped = candidate_pack.to_this.remap(typed_word);
        if (remapped == typed_word) continue; // no change

        const std::string lower_remapped = to_lowercase(remapped);
        if (candidate_pack.dictionary.contains(lower_remapped)) {
            // Make sure the typed form is NOT in any pack (avoid false positives)
            if (typed_matches.empty()) {
                return DetectionResult{
                    Action::SwitchAndRetype,
                    candidate_pack.locale,
                    remapped
                };
            }
        }
    }

    return DetectionResult{Action::NoAction, {}, {}};
}

} // namespace clavi
