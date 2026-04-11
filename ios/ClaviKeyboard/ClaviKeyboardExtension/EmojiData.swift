import Foundation

/**
 * Emoji catalogue — mirrors EmojiData.kt.
 * Loads emoji/emoji.tsv from the extension bundle once, lazily.
 */
final class EmojiData {

    struct Entry {
        let emoji: String
        let keywords: [String]
    }

    static let shared = EmojiData()

    private(set) var entries: [Entry] = []
    private var isLoaded = false

    var all: [String] { entries.map { $0.emoji } }

    func load() {
        guard !isLoaded else { return }
        isLoaded = true
        guard let url = Bundle.main.url(forResource: "emoji", withExtension: "tsv",
                                        subdirectory: "emoji"),
              let content = try? String(contentsOf: url, encoding: .utf8) else { return }
        for line in content.components(separatedBy: "\n") {
            guard !line.hasPrefix("#"), !line.isEmpty else { continue }
            guard let tabIdx = line.firstIndex(of: "\t") else { continue }
            let emoji = String(line[..<tabIdx]).trimmingCharacters(in: .whitespaces)
            let kws = String(line[line.index(after: tabIdx)...])
                .trimmingCharacters(in: .whitespaces)
                .components(separatedBy: " ")
                .filter { !$0.isEmpty }
            if !emoji.isEmpty { entries.append(Entry(emoji: emoji, keywords: kws)) }
        }
    }

    func search(_ query: String) -> [String] {
        if query.isEmpty { return all }
        let q = query.lowercased()
        return entries.filter { e in e.keywords.contains { $0.hasPrefix(q) } }.map { $0.emoji }
    }
}
