import UIKit

/**
 * Clipboard history for iOS keyboard extension.
 * UIKit keyboard extensions have elevated clipboard access.
 */
class ClipboardHistory {

    var onChange: (() -> Void)?

    private let maxItems = 20
    private var clips: [String] = []

    var recent: [String] { clips }

    func refresh() {
        guard let text = UIPasteboard.general.string?.trimmingCharacters(in: .whitespacesAndNewlines),
              !text.isEmpty, text.count <= 2000 else { return }
        add(text)
    }

    private func add(_ text: String) {
        clips.removeAll { $0 == text }
        clips.insert(text, at: 0)
        if clips.count > maxItems { clips.removeLast() }
        onChange?()
    }

    func fullText(at index: Int) -> String? {
        clips.indices.contains(index) ? clips[index] : nil
    }

    func displayLabel(_ text: String) -> String {
        let single = text.replacingOccurrences(of: "\n", with: " ")
        return single.count <= 40 ? single : String(single.prefix(39)) + "…"
    }

    func remove(at index: Int) {
        guard clips.indices.contains(index) else { return }
        clips.remove(at: index)
        onChange?()
    }

    func clear() {
        clips.removeAll()
        onChange?()
    }
}
