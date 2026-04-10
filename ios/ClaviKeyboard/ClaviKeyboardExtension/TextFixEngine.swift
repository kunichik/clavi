import Foundation

/**
 * Text fix engine — iOS mirror of TextFixEngine.kt (v0.4).
 *
 * Principle: FIX, DON'T REWRITE.
 * - Correct obvious typos and common spelling errors
 * - Fix punctuation spacing
 * - NEVER change word choice, sentence structure, tone, or style
 * - If unsure → return nil
 */
struct TextFixEngine {

    struct Fix {
        let original: String   // full textBefore BEFORE fix
        let fixed: String      // full textBefore AFTER fix
        let description: String
    }

    static func analyze(_ textBefore: String) -> Fix? {
        guard !textBefore.trimmingCharacters(in: .whitespaces).isEmpty else { return nil }

        if let fix = fixDoubleSpace(textBefore)         { return fix }
        if let fix = fixSpaceBeforePunct(textBefore)    { return fix }
        if let fix = fixMissingSpaceAfterPunct(textBefore) { return fix }
        if let fix = fixUkrainianTypo(textBefore)       { return fix }
        if let fix = fixEnglishTypo(textBefore)         { return fix }
        if let fix = fixRepeatedChars(textBefore)       { return fix }
        if let fix = fixAccidentalCaps(textBefore)      { return fix }

        return nil
    }

    // MARK: - Fix implementations

    private static func fixDoubleSpace(_ text: String) -> Fix? {
        guard text.hasSuffix("  ") else { return nil }
        let fixed = text.replacingOccurrences(of: "  +$", with: " ", options: .regularExpression)
        return Fix(original: text, fixed: fixed, description: "double space")
    }

    private static func fixSpaceBeforePunct(_ text: String) -> Fix? {
        guard let range = text.range(of: #"\w( )([.,!?;:])$"#, options: .regularExpression) else { return nil }
        // Find the space group
        guard let spaceRange = text.range(of: #"(?<=\w)( )(?=[.,!?;:]$)"#, options: .regularExpression) else { return nil }
        var fixed = text
        fixed.removeSubrange(spaceRange)
        return Fix(original: text, fixed: fixed, description: "space before punctuation")
    }

    private static func fixMissingSpaceAfterPunct(_ text: String) -> Fix? {
        let pattern = #"[a-zA-Zа-яА-ЯіІїЇєЄґҐ]([.!?])([a-zA-Zа-яА-ЯіІїЇєЄґҐ])$"#
        guard let match = text.range(of: pattern, options: .regularExpression) else { return nil }
        // Find the second letter group and insert space before it
        guard let letterRange = text.range(of: #"(?<=[.!?])([a-zA-Zа-яА-ЯіІїЇєЄґҐ])$"#, options: .regularExpression) else { return nil }
        var fixed = text
        fixed.insert(" ", at: letterRange.lowerBound)
        return Fix(original: text, fixed: fixed, description: "missing space after sentence")
    }

    private static func fixUkrainianTypo(_ text: String) -> Fix? {
        guard let lastWord = extractLastWord(text) else { return nil }
        guard let corrected = ukTypos[lastWord.lowercased()] else { return nil }
        let prefix = String(text.dropLast(lastWord.count))
        let replacement = lastWord.first?.isUppercase == true
            ? corrected.prefix(1).uppercased() + corrected.dropFirst()
            : corrected
        return Fix(original: text, fixed: prefix + replacement, description: "typo: \(lastWord) → \(corrected)")
    }

    private static func fixEnglishTypo(_ text: String) -> Fix? {
        guard let lastWord = extractLastWord(text) else { return nil }
        guard let corrected = enTypos[lastWord.lowercased()] else { return nil }
        let prefix = String(text.dropLast(lastWord.count))
        let replacement = lastWord.first?.isUppercase == true
            ? corrected.prefix(1).uppercased() + corrected.dropFirst()
            : corrected
        return Fix(original: text, fixed: prefix + replacement, description: "typo: \(lastWord) → \(corrected)")
    }

    private static func fixRepeatedChars(_ text: String) -> Fix? {
        guard let lastWord = extractLastWord(text), lastWord.count >= 4 else { return nil }
        // Replace 3+ repeated chars with 1 (or 2 for Ukrainian doubled consonants)
        let pattern = #"(.)\1{2,}"#
        guard let fixed = try? NSRegularExpression(pattern: pattern).stringByReplacingMatches(
            in: lastWord,
            range: NSRange(lastWord.startIndex..., in: lastWord),
            withTemplate: { (match: String) -> String in
                let ch = String(match.first!)
                let ukDoubled = ["н","л","т","с","з"]
                return ukDoubled.contains(ch) ? ch + ch : ch
            }("")
        ) else { return nil }
        // NSRegularExpression doesn't support closure replacement — use manual approach
        return fixRepeatedCharsManual(text, lastWord: lastWord)
    }

    private static func fixRepeatedCharsManual(_ text: String, lastWord: String) -> Fix? {
        var fixed = ""
        var prev: Character? = nil
        var repeatCount = 0

        for ch in lastWord {
            if ch == prev {
                repeatCount += 1
                let ukDoubled: Set<Character> = ["н","л","т","с","з"]
                if repeatCount == 2 && ukDoubled.contains(ch) {
                    fixed.append(ch)  // allow one double
                } else if repeatCount >= 3 {
                    // skip — too many repeats
                } else {
                    fixed.append(ch)
                }
            } else {
                fixed.append(ch)
                prev = ch
                repeatCount = 1
            }
        }

        guard fixed != lastWord else { return nil }
        let prefix = String(text.dropLast(lastWord.count))
        return Fix(original: text, fixed: prefix + fixed, description: "repeated characters")
    }

    private static func fixAccidentalCaps(_ text: String) -> Fix? {
        guard let word = extractLastWord(text), word.count >= 4 else { return nil }
        let chars = Array(word)
        // Pattern: first two chars uppercase, at least one lowercase after (e.g. "ПРивіт", "HEllo")
        guard chars[0].isUppercase && chars[1].isUppercase else { return nil }
        guard chars.dropFirst(2).contains(where: { $0.isLowercase }) else { return nil }
        // Fix: keep first char uppercase, lowercase the rest
        let fixed = String(chars[0]) + String(word.dropFirst()).lowercased()
        guard fixed != word else { return nil }
        let prefix = String(text.dropLast(word.count))
        let shortWord = word.count > 6 ? String(word.prefix(6)) + "…" : word
        let shortFixed = fixed.count > 6 ? String(fixed.prefix(6)) + "…" : fixed
        return Fix(original: text, fixed: prefix + fixed,
                   description: "caps: \(shortWord) → \(shortFixed)")
    }

    private static func extractLastWord(_ text: String) -> String? {
        let trimmed = text.trimmingCharacters(in: .whitespaces)
        let pattern = #"[\p{L}\p{M}'\-]+$"#
        guard let range = trimmed.range(of: pattern, options: .regularExpression) else { return nil }
        let word = String(trimmed[range])
        return word.count >= 2 ? word : nil
    }

    // MARK: - Typo dictionaries

    private static let ukTypos: [String: String] = [
        "pryvit":    "привіт",
        "dyakuyu":   "дякую",
        "будте":     "будьте",
        "памятати":  "пам'ятати",
        "компютер":  "комп'ютер",
        "звязок":    "зв'язок",
        "памятка":   "пам'ятка",
        "тчто":      "що",
        "зто":       "це",
    ]

    private static let enTypos: [String: String] = [
        "teh":         "the",
        "hte":         "the",
        "adn":         "and",
        "nad":         "and",
        "taht":        "that",
        "thta":        "that",
        "waht":        "what",
        "recieve":     "receive",
        "recieved":    "received",
        "beleive":     "believe",
        "freind":      "friend",
        "wierd":       "weird",
        "definately":  "definitely",
        "seperate":    "separate",
        "occured":     "occurred",
        "untill":      "until",
        "accomodate":  "accommodate",
        "occurence":   "occurrence",
        "tommorow":    "tomorrow",
        "tommorrow":   "tomorrow",
    ]
}
