import Foundation

/**
 * Smart diacritics engine — mirrors DiacriticsEngine.kt exactly.
 * Suggests diacritic variants for a base letter in a given locale.
 * Variants are frequency-ordered (most common first).
 */
struct DiacriticsEngine {

    static func suggest(_ char: Character, locale: String) -> [String] {
        let lang = locale.lowercased().components(separatedBy: CharacterSet(charactersIn: "_-")).first ?? locale
        guard let table = tables[lang] else { return [] }
        let lower = Character(char.lowercased())
        guard let variants = table[lower] else { return [] }
        return char.isUppercase ? variants.map { $0.uppercased() } : variants
    }

    static func hasVariants(_ char: Character, locale: String) -> Bool {
        let lang = locale.lowercased().components(separatedBy: CharacterSet(charactersIn: "_-")).first ?? locale
        guard let table = tables[lang] else { return false }
        return table[Character(char.lowercased())] != nil
    }

    private static let tables: [String: [Character: [String]]] = [
        "pt": [
            "a": ["ã","â","á","à","a"],
            "e": ["é","ê","e"],
            "i": ["í","i"],
            "o": ["ô","ó","õ","o"],
            "u": ["ú","ü","u"],
            "c": ["ç","c"],
            "n": ["ñ","n"],
        ],
        "de": [
            "a": ["ä","a"],
            "o": ["ö","o"],
            "u": ["ü","u"],
            "s": ["ß","s"],
        ],
        "no": [
            "a": ["å","a"],
            "e": ["æ","e"],
            "o": ["ø","o"],
        ],
        "nb": [
            "a": ["å","a"],
            "e": ["æ","e"],
            "o": ["ø","o"],
        ],
        "fr": [
            "e": ["é","è","ê","ë","e"],
            "a": ["à","â","a"],
            "c": ["ç","c"],
            "i": ["î","ï","i"],
            "o": ["ô","o"],
            "u": ["ù","û","ü","u"],
        ],
        "es": [
            "n": ["ñ","n"],
            "a": ["á","a"],
            "e": ["é","e"],
            "i": ["í","i"],
            "o": ["ó","o"],
            "u": ["ú","ü","u"],
        ],
        "sv": [
            "a": ["å","ä","a"],
            "o": ["ö","o"],
            "u": ["ü","u"],
        ],
        "fi": [
            "a": ["ä","a"],
            "o": ["ö","o"],
        ],
        "pl": [
            "a": ["ą","a"],
            "c": ["ć","c"],
            "e": ["ę","e"],
            "l": ["ł","l"],
            "n": ["ń","n"],
            "o": ["ó","o"],
            "s": ["ś","s"],
            "z": ["ź","ż","z"],
        ],
    ]
}
