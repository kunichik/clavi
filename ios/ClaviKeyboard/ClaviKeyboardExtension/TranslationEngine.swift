import Foundation

/**
 * Two-tier translation engine (iOS mirror of TranslationEngine.kt).
 *
 * Tier 1: Static TSV word dictionary bundled in Resources/translate/{src}-{tgt}.tsv
 *         Zero download, word-level, instant.
 * Tier 2: Apple Translation framework (iOS 17+), sentence-level.
 *         Falls back to Tier 1 if iOS < 17 or translation fails.
 */
struct TranslationEngine {

    struct TranslationSuggestion {
        let original: String    // phrase that was translated
        let translated: String  // translation result
        let sourceLang: String
        let targetLang: String
    }

    let sourceLang: String
    let targetLang: String

    private let dictionary: [String: String]

    init(sourceLang: String, targetLang: String) {
        self.sourceLang = sourceLang
        self.targetLang = targetLang
        self.dictionary = TranslationEngine.loadDict(src: sourceLang, tgt: targetLang)
    }

    /**
     * Async translate. iOS 17+: Apple Translation framework (sentence-level).
     * Older iOS or failure: falls back to word-level TSV lookup.
     * Calls [completion] on the main thread.
     */
    func translate(_ phraseBeforeCursor: String, completion: @escaping (TranslationSuggestion?) -> Void) {
        guard let phrase = extractPhrase(phraseBeforeCursor) else {
            completion(nil)
            return
        }

        if #available(iOS 17, *) {
            Task {
                do {
                    let result = try await appleTranslate(phrase)
                    await MainActor.run {
                        completion(TranslationSuggestion(
                            original: phrase, translated: result,
                            sourceLang: sourceLang, targetLang: targetLang))
                    }
                } catch {
                    // Fall back to Tier 1
                    let word = lastWord(of: phrase)
                    let result = dictionary[word.lowercased()]
                    await MainActor.run {
                        completion(result.map {
                            TranslationSuggestion(original: word, translated: $0,
                                                  sourceLang: sourceLang, targetLang: targetLang)
                        })
                    }
                }
            }
        } else {
            // iOS < 17: word-level TSV only
            let word = lastWord(of: phrase)
            if let result = dictionary[word.lowercased()] {
                completion(TranslationSuggestion(original: word, translated: result,
                                                 sourceLang: sourceLang, targetLang: targetLang))
            } else {
                completion(nil)
            }
        }
    }

    // MARK: - Apple Translation (iOS 17+)

    @available(iOS 17, *)
    private func appleTranslate(_ text: String) async throws -> String {
        let src = Locale.Language(identifier: sourceLang)
        let tgt = Locale.Language(identifier: targetLang)
        let config = TranslationSession.Configuration(source: src, target: tgt)
        let session = TranslationSession(configuration: config)
        let response = try await session.translate(text)
        return response.targetText
    }

    // MARK: - Phrase extraction

    private func extractPhrase(_ text: String) -> String? {
        let trimmed = text.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else { return nil }
        let sentenceEnders = CharacterSet(charactersIn: ".!?\n")
        let phrase: String
        if let range = trimmed.rangeOfCharacter(from: sentenceEnders, options: .backwards) {
            phrase = String(trimmed[trimmed.index(after: range.lowerBound)...])
                .trimmingCharacters(in: .whitespaces)
        } else {
            phrase = trimmed
        }
        return phrase.count > 2 ? phrase : nil
    }

    private func lastWord(of phrase: String) -> String {
        phrase.trimmingCharacters(in: .whitespaces)
            .components(separatedBy: " ")
            .last(where: { !$0.isEmpty }) ?? phrase
    }

    // MARK: - TSV dictionary loader

    private static func loadDict(src: String, tgt: String) -> [String: String] {
        guard let url = Bundle.main.url(forResource: "\(src)-\(tgt)", withExtension: "tsv",
                                        subdirectory: "translate"),
              let content = try? String(contentsOf: url, encoding: .utf8) else { return [:] }
        var dict: [String: String] = [:]
        for line in content.components(separatedBy: "\n") {
            guard !line.hasPrefix("#"), !line.isEmpty,
                  let tabIdx = line.firstIndex(of: "\t") else { continue }
            let key = String(line[..<tabIdx])
            let value = String(line[line.index(after: tabIdx)...])
            dict[key] = value
        }
        return dict
    }
}
