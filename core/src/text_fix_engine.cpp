#include "clavi/text_fix_engine.hpp"
#include "clavi/utf8_utils.hpp"

#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace clavi {

namespace {

// ── Codepoint helpers ─────────────────────────────────────────────────────────

bool is_upper_cp(uint32_t cp) noexcept {
    if (cp >= 'A' && cp <= 'Z') return true;
    if (cp >= 0x0410 && cp <= 0x042F) return true;  // А–Я
    return cp == 0x0404 || cp == 0x0406 || cp == 0x0407 || cp == 0x0490; // Є І Ї Ґ
}

bool is_lower_cp(uint32_t cp) noexcept {
    if (cp >= 'a' && cp <= 'z') return true;
    if (cp >= 0x0430 && cp <= 0x044F) return true;  // а–я
    return cp == 0x0454 || cp == 0x0456 || cp == 0x0457 || cp == 0x0491; // є і ї ґ
}

std::string capitalize_first(std::string_view s) {
    if (s.empty()) return {};
    const char* p = s.data();
    const char* end = p + s.size();
    std::string out;
    out.reserve(s.size());
    const uint32_t first = utf8::decode(p, end);
    uint32_t up = first;
    if (first >= 'a' && first <= 'z')              up = first - 32;
    else if (first >= 0x0430 && first <= 0x044F)   up = first - 0x20;
    else if (first == 0x0454) up = 0x0404;  // є → Є
    else if (first == 0x0456) up = 0x0406;  // і → І
    else if (first == 0x0457) up = 0x0407;  // ї → Ї
    else if (first == 0x0491) up = 0x0490;  // ґ → Ґ
    utf8::encode(up, out);
    out.append(p, end);
    return out;
}

// ── Typo dictionaries ─────────────────────────────────────────────────────────

const std::unordered_map<std::string, std::string>& en_typos() {
    static const std::unordered_map<std::string, std::string> t = {
        // Finger-roll transpositions
        {"teh",           "the"},
        {"hte",           "the"},
        {"adn",           "and"},
        {"nad",           "and"},
        {"taht",          "that"},
        {"thta",          "that"},
        {"waht",          "what"},
        {"hwat",          "what"},
        {"yuo",           "you"},
        // Missing apostrophes
        {"youre",         "you're"},
        {"dont",          "don't"},
        {"doesnt",        "doesn't"},
        {"didnt",         "didn't"},
        {"wont",          "won't"},
        {"cant",          "can't"},
        {"isnt",          "isn't"},
        {"wasnt",         "wasn't"},
        {"werent",        "weren't"},
        {"havent",        "haven't"},
        {"hasnt",         "hasn't"},
        {"wouldnt",       "wouldn't"},
        {"couldnt",       "couldn't"},
        {"shouldnt",      "shouldn't"},
        {"im",            "I'm"},
        {"ive",           "I've"},
        {"id",            "I'd"},
        {"ill",           "I'll"},
        // i-before-e
        {"recieve",       "receive"},
        {"recieved",      "received"},
        {"beleive",       "believe"},
        {"beleived",      "believed"},
        {"freind",        "friend"},
        {"wierd",         "weird"},
        {"acheive",       "achieve"},
        {"acheived",      "achieved"},
        {"peice",         "piece"},
        {"releive",       "relieve"},
        // Double-letter confusion
        {"occured",       "occurred"},
        {"occurence",     "occurrence"},
        {"accomodate",    "accommodate"},
        {"embarass",      "embarrass"},
        {"harrass",       "harass"},
        {"necesary",      "necessary"},
        {"neccessary",    "necessary"},
        {"posession",     "possession"},
        {"agressive",     "aggressive"},
        {"profesional",   "professional"},
        {"recomend",      "recommend"},
        // -ately / -itely
        {"definately",    "definitely"},
        {"definetly",     "definitely"},
        {"absolutley",    "absolutely"},
        {"immediatley",   "immediately"},
        // -ate / -ete confusion
        {"seperate",      "separate"},
        {"seperated",     "separated"},
        {"desparate",     "desperate"},
        // Common misspellings
        {"untill",        "until"},
        {"tommorow",      "tomorrow"},
        {"tommorrow",     "tomorrow"},
        {"calender",      "calendar"},
        {"cemetary",      "cemetery"},
        {"goverment",     "government"},
        {"medecine",      "medicine"},
        {"millenium",     "millennium"},
        {"miniscule",     "minuscule"},
        {"mischevous",    "mischievous"},
        {"noticable",     "noticeable"},
        {"priviledge",    "privilege"},
        {"pronounciation","pronunciation"},
        {"publically",    "publicly"},
        {"questionaire",  "questionnaire"},
        {"succesful",     "successful"},
        {"suprise",       "surprise"},
        {"tendancy",      "tendency"},
        {"truely",        "truly"},
        {"useable",       "usable"},
        {"vaccum",        "vacuum"},
        {"withold",       "withhold"},
    };
    return t;
}

const std::unordered_map<std::string, std::string>& uk_typos() {
    // Keys are lowercase; values preserve canonical Ukrainian spelling.
    static const std::unordered_map<std::string, std::string> t = {
        // Translit artifacts (user typed Latin without translit mode)
        {"pryvit",       "привіт"},
        {"dyakuyu",      "дякую"},
        {"prosto",       "просто"},
        {"dobre",        "добре"},
        {"shcho",        "що"},
        {"yakshcho",     "якщо"},
        {"tomu",         "тому"},
        {"potim",        "потім"},
        {"znayu",        "знаю"},
        {"rozumiyu",     "розумію"},
        // Missing apostrophe — the most common Ukrainian typo category
        {"памятати",     "пам'ятати"},
        {"памятка",      "пам'ятка"},
        {"памяти",       "пам'яті"},
        {"память",       "пам'ять"},
        {"компютер",     "комп'ютер"},
        {"звязок",       "зв'язок"},
        {"звязати",      "зв'язати"},
        {"мяч",          "м'яч"},
        {"обєкт",        "об'єкт"},
        {"субєкт",       "суб'єкт"},
        {"девять",       "дев'ять"},
        {"пять",         "п'ять"},
        {"вязати",       "в'язати"},
        {"вязаний",      "в'язаний"},
        {"зєднати",      "з'єднати"},
        // Missing soft sign
        {"будте",        "будьте"},
    };
    return t;
}

// Ukrainian doubled consonants that legitimately appear as doubles (keep 2, not 1)
bool is_uk_double_consonant(uint32_t cp) noexcept {
    return cp == 0x043D  // н
        || cp == 0x043B  // л
        || cp == 0x0442  // т
        || cp == 0x0441  // с
        || cp == 0x0437  // з
        || cp == 0x0439  // й
        || cp == 0x0440  // р
        || cp == 0x0434; // д
}

} // anonymous namespace

// ── Public API ────────────────────────────────────────────────────────────────

std::optional<TextFixEngine::Fix> TextFixEngine::check_word(std::string_view word) {
    if (word.size() < 2) return std::nullopt;
    if (auto f = check_en_typo(word))         return f;
    if (auto f = check_uk_typo(word))         return f;
    if (auto f = check_repeated_chars(word))  return f;
    if (auto f = check_accidental_caps(word)) return f;
    return std::nullopt;
}

// ── Private checks ────────────────────────────────────────────────────────────

std::optional<TextFixEngine::Fix> TextFixEngine::check_en_typo(std::string_view word) {
    const std::string lower = utf8::to_lower(word);
    const auto& t = en_typos();
    const auto it = t.find(lower);
    if (it == t.end()) return std::nullopt;

    const char* p = word.data();
    const char* end = p + word.size();
    const uint32_t first = utf8::decode(p, end);
    const std::string corrected =
        is_upper_cp(first) ? capitalize_first(it->second) : it->second;

    if (corrected == std::string(word)) return std::nullopt;
    return Fix{std::string(word), corrected};
}

std::optional<TextFixEngine::Fix> TextFixEngine::check_uk_typo(std::string_view word) {
    const std::string lower = utf8::to_lower(word);
    const auto& t = uk_typos();
    const auto it = t.find(lower);
    if (it == t.end()) return std::nullopt;

    const char* p = word.data();
    const char* end = p + word.size();
    const uint32_t first = utf8::decode(p, end);
    const std::string corrected =
        is_upper_cp(first) ? capitalize_first(it->second) : it->second;

    if (corrected == std::string(word)) return std::nullopt;
    return Fix{std::string(word), corrected};
}

std::optional<TextFixEngine::Fix> TextFixEngine::check_repeated_chars(std::string_view word) {
    if (word.size() < 4) return std::nullopt;

    std::vector<uint32_t> cps;
    cps.reserve(word.size());
    const char* p = word.data();
    const char* end = p + word.size();
    while (p < end) {
        const uint32_t cp = utf8::decode(p, end);
        if (cp == 0xFFFD) return std::nullopt;
        cps.push_back(cp);
    }
    if (cps.size() < 4) return std::nullopt;

    std::vector<uint32_t> out_cps;
    out_cps.reserve(cps.size());
    bool changed = false;
    std::size_t i = 0;
    while (i < cps.size()) {
        const uint32_t cp = cps[i];
        std::size_t run = 1;
        while (i + run < cps.size() && cps[i + run] == cp) run++;
        if (run >= 3) {
            changed = true;
            const std::size_t keep = is_uk_double_consonant(cp) ? 2 : 1;
            for (std::size_t j = 0; j < keep; j++) out_cps.push_back(cp);
        } else {
            for (std::size_t j = 0; j < run; j++) out_cps.push_back(cp);
        }
        i += run;
    }

    if (!changed) return std::nullopt;

    std::string corrected;
    corrected.reserve(word.size());
    for (const uint32_t cp : out_cps) utf8::encode(cp, corrected);

    if (corrected == word) return std::nullopt;
    return Fix{std::string(word), corrected};
}

std::optional<TextFixEngine::Fix> TextFixEngine::check_accidental_caps(std::string_view word) {
    if (word.size() < 4) return std::nullopt;

    const char* p = word.data();
    const char* end = p + word.size();
    const uint32_t cp0 = utf8::decode(p, end);
    if (cp0 == 0xFFFD) return std::nullopt;
    const uint32_t cp1 = utf8::decode(p, end);
    if (cp1 == 0xFFFD) return std::nullopt;

    if (!is_upper_cp(cp0) || !is_upper_cp(cp1)) return std::nullopt;

    bool has_lower = false;
    std::vector<uint32_t> rest;
    while (p < end) {
        const uint32_t cp = utf8::decode(p, end);
        if (cp == 0xFFFD) return std::nullopt;
        rest.push_back(cp);
        if (is_lower_cp(cp)) has_lower = true;
    }

    if (!has_lower) return std::nullopt;  // all-caps (HTTP, NATO) — leave alone

    std::string corrected;
    corrected.reserve(word.size());
    utf8::encode(cp0, corrected);                         // first char: keep uppercase
    utf8::encode(utf8::to_lower_cp(cp1), corrected);     // second char: lowercase
    for (const uint32_t cp : rest)
        utf8::encode(utf8::to_lower_cp(cp), corrected);  // rest: lowercase

    if (corrected == std::string(word)) return std::nullopt;
    return Fix{std::string(word), corrected};
}

} // namespace clavi
