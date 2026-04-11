import Foundation

/**
 * Word prediction engine (v1.5) — mirrors WordPredictionEngine.kt exactly.
 *
 * Loads predict/{lang}.tsv from the extension bundle.
 * Format: trigger<TAB>sugg1<TAB>sugg2<TAB>sugg3
 * Empty trigger = default / sentence-start suggestions.
 *
 * Supports .en and .uk; other languages return [].
 */
struct WordPredictionEngine {

    // Cache: asset key → bigram dict (populated on first use per language)
    private var cache: [String: [String: [String]]] = [:]

    mutating func predict(_ textBefore: String, language: Language, maxResults: Int = 3) -> [String] {
        guard let assetKey = Self.assetKey(for: language) else { return [] }
        let dict = loadedDict(assetKey)
        if dict.isEmpty { return [] }

        let lastWord = extractLastWord(from: textBefore).lowercased()
        if let hits = dict[lastWord], !hits.isEmpty {
            return Array(hits.prefix(maxResults))
        }
        // Fall back to default context suggestions
        return Array((dict[""] ?? []).prefix(maxResults))
    }

    // MARK: - Private

    private static func assetKey(for language: Language) -> String? {
        switch language {
        case .en: return "en"
        case .uk: return "uk"
        default:  return nil
        }
    }

    private mutating func loadedDict(_ key: String) -> [String: [String]] {
        if let cached = cache[key] { return cached }
        let dict = Self.loadDict(key)
        cache[key] = dict
        return dict
    }

    private func extractLastWord(from text: String) -> String {
        let trimmed = text.trimmingCharacters(in: .init(charactersIn: " \n"))
        if trimmed.isEmpty { return "" }
        return trimmed.components(separatedBy: CharacterSet(charactersIn: " \n")).last ?? ""
    }

    private static func loadDict(_ lang: String) -> [String: [String]] {
        guard let url = Bundle.main.url(forResource: lang, withExtension: "tsv",
                                        subdirectory: "predict"),
              let content = try? String(contentsOf: url, encoding: .utf8) else { return [:] }

        var result: [String: [String]] = [:]
        for line in content.components(separatedBy: "\n") {
            guard !line.hasPrefix("#"), !line.isEmpty else { continue }
            let parts = line.components(separatedBy: "\t")
            guard parts.count >= 2 else { continue }
            let trigger = parts[0]
            let suggestions = parts.dropFirst().filter { !$0.isEmpty }
            if !suggestions.isEmpty {
                result[trigger] = Array(suggestions)
            }
        }
        return result
    }
}
