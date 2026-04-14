#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace clavi {

/**
 * TextFixEngine — word-level typo correction for the desktop daemon.
 *
 * Principle: FIX, DON'T REWRITE.
 *  - Correct obvious EN + UK typos (small high-confidence dictionary)
 *  - Fix accidental repeated characters (3+ identical in a row)
 *  - Fix accidental CapsLock (two leading uppercase + mixed-case rest)
 *  - NEVER change word choice, sentence structure, tone, or style
 *  - If unsure → return std::nullopt (no fix)
 *
 * check_word() receives a single completed word (without trailing space or
 * punctuation).  All checks are O(1) or O(word length) — well under 1 ms.
 */
class TextFixEngine {
public:
    struct Fix {
        std::string original;   // the word as typed (the typo)
        std::string corrected;  // the corrected form
    };

    /// Analyse a completed word and return a high-confidence fix, or nullopt.
    [[nodiscard]] static std::optional<Fix> check_word(std::string_view word);

private:
    static std::optional<Fix> check_en_typo(std::string_view word);
    static std::optional<Fix> check_uk_typo(std::string_view word);
    static std::optional<Fix> check_repeated_chars(std::string_view word);
    static std::optional<Fix> check_accidental_caps(std::string_view word);
};

} // namespace clavi
