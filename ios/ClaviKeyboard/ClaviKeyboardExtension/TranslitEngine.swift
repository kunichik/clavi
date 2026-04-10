import Foundation

/**
 * KMU 2010 Ukrainian transliteration engine.
 * Mirrors TranslitEngine.kt — longest-match-first.
 */
struct TranslitEngine {

    private struct Rule {
        let latin: String
        let cyrillic: String
    }

    private let rules: [Rule] = [
        // Trigraphs
        Rule(latin: "shch", cyrillic: "щ"), Rule(latin: "Shch", cyrillic: "Щ"), Rule(latin: "SHCH", cyrillic: "Щ"),
        // Digraphs
        Rule(latin: "zh", cyrillic: "ж"), Rule(latin: "Zh", cyrillic: "Ж"), Rule(latin: "ZH", cyrillic: "Ж"),
        Rule(latin: "kh", cyrillic: "х"), Rule(latin: "Kh", cyrillic: "Х"), Rule(latin: "KH", cyrillic: "Х"),
        Rule(latin: "ts", cyrillic: "ц"), Rule(latin: "Ts", cyrillic: "Ц"), Rule(latin: "TS", cyrillic: "Ц"),
        Rule(latin: "ch", cyrillic: "ч"), Rule(latin: "Ch", cyrillic: "Ч"), Rule(latin: "CH", cyrillic: "Ч"),
        Rule(latin: "sh", cyrillic: "ш"), Rule(latin: "Sh", cyrillic: "Ш"), Rule(latin: "SH", cyrillic: "Ш"),
        Rule(latin: "iu", cyrillic: "ю"), Rule(latin: "Iu", cyrillic: "Ю"), Rule(latin: "IU", cyrillic: "Ю"),
        Rule(latin: "ia", cyrillic: "я"), Rule(latin: "Ia", cyrillic: "Я"), Rule(latin: "IA", cyrillic: "Я"),
        Rule(latin: "ie", cyrillic: "є"), Rule(latin: "Ie", cyrillic: "Є"), Rule(latin: "IE", cyrillic: "Є"),
        Rule(latin: "yi", cyrillic: "ї"), Rule(latin: "Yi", cyrillic: "Ї"), Rule(latin: "YI", cyrillic: "Ї"),
        // Apostrophe
        Rule(latin: "'", cyrillic: "\u{02BC}"),
        // Single chars
        Rule(latin: "a", cyrillic: "а"), Rule(latin: "A", cyrillic: "А"),
        Rule(latin: "b", cyrillic: "б"), Rule(latin: "B", cyrillic: "Б"),
        Rule(latin: "v", cyrillic: "в"), Rule(latin: "V", cyrillic: "В"),
        Rule(latin: "h", cyrillic: "г"), Rule(latin: "H", cyrillic: "Г"),
        Rule(latin: "g", cyrillic: "ґ"), Rule(latin: "G", cyrillic: "Ґ"),
        Rule(latin: "d", cyrillic: "д"), Rule(latin: "D", cyrillic: "Д"),
        Rule(latin: "e", cyrillic: "е"), Rule(latin: "E", cyrillic: "Е"),
        Rule(latin: "z", cyrillic: "з"), Rule(latin: "Z", cyrillic: "З"),
        Rule(latin: "y", cyrillic: "и"), Rule(latin: "Y", cyrillic: "И"),
        Rule(latin: "i", cyrillic: "і"), Rule(latin: "I", cyrillic: "І"),
        Rule(latin: "j", cyrillic: "й"), Rule(latin: "J", cyrillic: "Й"),
        Rule(latin: "k", cyrillic: "к"), Rule(latin: "K", cyrillic: "К"),
        Rule(latin: "l", cyrillic: "л"), Rule(latin: "L", cyrillic: "Л"),
        Rule(latin: "m", cyrillic: "м"), Rule(latin: "M", cyrillic: "М"),
        Rule(latin: "n", cyrillic: "н"), Rule(latin: "N", cyrillic: "Н"),
        Rule(latin: "o", cyrillic: "о"), Rule(latin: "O", cyrillic: "О"),
        Rule(latin: "p", cyrillic: "п"), Rule(latin: "P", cyrillic: "П"),
        Rule(latin: "r", cyrillic: "р"), Rule(latin: "R", cyrillic: "Р"),
        Rule(latin: "s", cyrillic: "с"), Rule(latin: "S", cyrillic: "С"),
        Rule(latin: "t", cyrillic: "т"), Rule(latin: "T", cyrillic: "Т"),
        Rule(latin: "u", cyrillic: "у"), Rule(latin: "U", cyrillic: "У"),
        Rule(latin: "f", cyrillic: "ф"), Rule(latin: "F", cyrillic: "Ф"),
        Rule(latin: "x", cyrillic: "кс"), Rule(latin: "X", cyrillic: "КС"),
        Rule(latin: "c", cyrillic: "с"), Rule(latin: "C", cyrillic: "С"),
        Rule(latin: "q", cyrillic: "к"), Rule(latin: "Q", cyrillic: "К"),
        Rule(latin: "w", cyrillic: "в"), Rule(latin: "W", cyrillic: "В"),
    ]

    private var maxLen: Int { rules.map { $0.latin.count }.max() ?? 1 }

    /// Try to transliterate the beginning of `buffer`. Returns (result, charsConsumed) or nil.
    func tryTransliterate(_ buffer: String) -> (String, Int)? {
        let maxLen = self.maxLen
        for len in stride(from: min(maxLen, buffer.count), through: 1, by: -1) {
            let candidate = String(buffer.prefix(len))
            if let rule = rules.first(where: { $0.latin == candidate }) {
                return (rule.cyrillic, len)
            }
        }
        return nil
    }

    func isDigraphStart(_ c: Character) -> Bool {
        "szktciy".contains(c.lowercased())
    }
}
