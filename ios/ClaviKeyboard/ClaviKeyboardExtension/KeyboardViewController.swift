import UIKit

/**
 * Clavi iOS Custom Keyboard Extension (v0.6)
 *
 * Features:
 * - Ukrainian ЙЦУКЕН + English QWERTY layouts
 * - Translit mode (KMU 2010): type Latin → get Ukrainian Cyrillic
 * - Language switch (UK ↔ EN)
 * - Clipboard history strip above keyboard
 * - Smart diacritics strip (variant suggestions after typing base letter)
 * - Text fix strip (typo correction, "fix don't rewrite")
 */
class KeyboardViewController: UIInputViewController {

    private var keyboardView: ClaviKeyboardView!
    private let translitEngine = TranslitEngine()
    private let clipboardHistory = ClipboardHistory()

    private var currentLanguage: Language = .uk
    private var shifted = false
    private var capsLock = false
    private var translitMode = false
    private var symbolsMode = false
    private var translitBuffer = ""

    // Language rotation
    private var activeLanguages: [Language] = [.uk, .en]
    private var currentLangIndex: Int = 0

    // Diacritics: userLocale from Settings, diacriticsLocale = effective (may be auto-set)
    private var userDiacriticsLocale: String? = nil
    private var diacriticsLocale: String? = nil

    private var translationEngine: TranslationEngine? = nil
    private var predictionEngine = WordPredictionEngine()

    // Spell engines — loaded in background; nil = not yet loaded
    private var spellEngineEn: SpellEngine? = nil
    private var spellEngineUk: SpellEngine? = nil
    private var lastAutocorrect: (original: String, replaced: String)? = nil

    private static let autocorrectThreshold: Float = 0.75

    // MARK: - Lifecycle

    override func viewDidLoad() {
        super.viewDidLoad()
        loadPreferences()
        setupKeyboard()
        clipboardHistory.onChange = { [weak self] in
            self?.keyboardView.clipItems = self?.clipboardHistory.recent ?? []
        }

        // Pre-warm spell engines — each SpellEngine loads its dict asynchronously
        spellEngineEn = SpellEngine(lang: "en")
        spellEngineUk = SpellEngine(lang: "uk")
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        loadPreferences()

        // Password field detection: hide sensitive strips for privacy
        let isPassword = textDocumentProxy.keyboardType == .some(.asciiCapableNumberPad) ||
            textDocumentProxy.textContentType == .password ||
            textDocumentProxy.textContentType == .newPassword
        if isPassword {
            keyboardView.clipItems = []
            keyboardView.translationSuggestion = nil
            keyboardView.predictionItems = []
            if translitMode { handleTranslitToggle() }
        } else {
            clipboardHistory.refresh()
            keyboardView.clipItems = clipboardHistory.recent
        }
    }

    // MARK: - Preferences

    private func loadPreferences() {
        let defaults = UserDefaults(suiteName: "group.com.clavi.keyboard") ?? .standard
        userDiacriticsLocale = defaults.string(forKey: "diacritics_locale")
        diacriticsLocale = userDiacriticsLocale

        let savedLang = defaults.string(forKey: "default_language") ?? "uk"
        currentLanguage = Language(rawValue: savedLang.lowercased()) ?? .uk

        // Load active languages (array of rawValues stored as [String])
        let defaultActive = ["uk", "en"]
        let savedActive = (defaults.array(forKey: "active_languages") as? [String]) ?? defaultActive
        // Preserve canonical Language.allCases order
        activeLanguages = Language.allCases.filter { savedActive.contains($0.rawValue) }
        if activeLanguages.isEmpty { activeLanguages = [.uk, .en] }
        currentLangIndex = activeLanguages.firstIndex(of: currentLanguage) ?? 0

        let transSrc = defaults.string(forKey: "translation_source_lang")
        let transTgt = defaults.string(forKey: "translation_target_lang")
        translationEngine = (transSrc != nil && transTgt != nil)
            ? TranslationEngine(sourceLang: transSrc!, targetLang: transTgt!)
            : nil
    }

    // MARK: - Setup

    private func setupKeyboard() {
        keyboardView = ClaviKeyboardView()
        keyboardView.translatesAutoresizingMaskIntoConstraints = false
        keyboardView.delegate = self
        view.addSubview(keyboardView)

        NSLayoutConstraint.activate([
            keyboardView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            keyboardView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            keyboardView.topAnchor.constraint(equalTo: view.topAnchor),
            keyboardView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
        ])

        updateLayout()
    }

    // MARK: - Layout

    private func updateLayout() {
        let rows = symbolsMode
            ? KeyboardLayout.symbols
            : KeyboardLayout.layout(for: currentLanguage, shifted: shifted)
        keyboardView.rows = rows
        keyboardView.translitActive = translitMode
        keyboardView.currentLanguage = currentLanguage
    }

    // MARK: - Key handling

    private func handleKey(_ key: KeyDef) {
        switch key.action {
        case .character(let ch): handleCharacter(ch)
        case .shift:             handleShift()
        case .backspace:         handleBackspace()
        case .langSwitch:        handleLangSwitch()
        case .translit:          handleTranslitToggle()
        case .symbols:           handleSymbolsToggle()
        case .enter:             handleEnter()
        case .space:             handleCharacter(" ")
        }
    }

    private func handleCharacter(_ text: String) {
        if translitMode && currentLanguage == .en {
            translitBuffer += text
            flushTranslitBuffer(force: false)
            keyboardView.diacriticItems = []
            keyboardView.fixSuggestion = nil
            keyboardView.autocorrectReject = nil
            keyboardView.predictionItems = []
        } else {
            textDocumentProxy.insertText(text)
            showDiacriticsIfNeeded(text)

            if text == " " || text == "\n" {
                keyboardView.autocorrectReject = nil
                lastAutocorrect = nil
                let before = textDocumentProxy.documentContextBeforeInput ?? ""
                let fix = TextFixEngine.analyze(before)
                keyboardView.fixSuggestion = fix
                if fix == nil {
                    // Try autocorrect on the word just completed
                    let lastWord = extractLastCompletedWord(before)
                    let spell = spellEngine(for: currentLanguage)
                    if let lastWord = lastWord, let spell = spell,
                       shouldAutocorrect(lastWord), !spell.isCorrect(lastWord),
                       let sug = spell.correct(lastWord),
                       sug.confidence >= KeyboardViewController.autocorrectThreshold {
                        // Replace: delete (word + space) then commit (corrected + space)
                        for _ in 0..<(lastWord.count + 1) { textDocumentProxy.deleteBackward() }
                        textDocumentProxy.insertText(sug.word + text)
                        lastAutocorrect = (original: lastWord, replaced: sug.word)
                        keyboardView.autocorrectReject = (original: lastWord, replaced: sug.word)
                        // Strip taken by autocorrect undo — skip translation/prediction
                        if shifted && !capsLock { shifted = false; updateLayout() }
                        return
                    }
                    keyboardView.translationSuggestion = nil
                    translationEngine?.translate(before) { [weak self] suggestion in
                        self?.keyboardView.translationSuggestion = suggestion
                    }
                    keyboardView.predictionItems = predictionEngine.predict(before, language: currentLanguage)
                } else {
                    keyboardView.translationSuggestion = nil
                    keyboardView.predictionItems = []
                }
            } else {
                keyboardView.fixSuggestion = nil
                keyboardView.translationSuggestion = nil
                // Live completions as the user types
                let before = textDocumentProxy.documentContextBeforeInput ?? ""
                let partial = extractCurrentWord(before + text) ?? ""
                keyboardView.predictionItems = spellEngine(for: currentLanguage)?.complete(prefix: partial) ?? []
            }
        }

        if shifted && !capsLock {
            shifted = false
            updateLayout()
        }
    }

    private func showDiacriticsIfNeeded(_ text: String) {
        guard let locale = diacriticsLocale, let char = text.first, text.count == 1 else {
            keyboardView.diacriticItems = []
            return
        }
        let variants = DiacriticsEngine.suggest(char, locale: locale)
        keyboardView.diacriticItems = variants.count > 1 ? variants : []
    }

    private func flushTranslitBuffer(force: Bool) {
        while !translitBuffer.isEmpty {
            if let (result, consumed) = translitEngine.tryTransliterate(translitBuffer) {
                textDocumentProxy.insertText(result)
                translitBuffer = String(translitBuffer.dropFirst(consumed))
            } else if force || !translitEngine.isDigraphStart(translitBuffer.first!) {
                textDocumentProxy.insertText(String(translitBuffer.first!))
                translitBuffer = String(translitBuffer.dropFirst())
            } else {
                break
            }
        }
    }

    private func handleBackspace() {
        keyboardView.diacriticItems = []
        keyboardView.fixSuggestion = nil
        keyboardView.translationSuggestion = nil
        keyboardView.autocorrectReject = nil
        lastAutocorrect = nil
        keyboardView.predictionItems = []
        if !translitBuffer.isEmpty {
            translitBuffer = String(translitBuffer.dropLast())
        } else {
            textDocumentProxy.deleteBackward()
        }
    }

    private func handleEnter() {
        flushTranslitBuffer(force: true)
        textDocumentProxy.insertText("\n")
    }

    private func handleShift() {
        if shifted {
            capsLock.toggle()
            shifted = capsLock
        } else {
            shifted = true
            capsLock = false
        }
        updateLayout()
    }

    private func handleLangSwitch() {
        flushTranslitBuffer(force: true)
        currentLangIndex = (currentLangIndex + 1) % activeLanguages.count
        currentLanguage = activeLanguages[currentLangIndex]

        // Auto-enable diacritics for European languages; restore user setting for others
        diacriticsLocale = currentLanguage.diacriticsLocale ?? userDiacriticsLocale

        shifted = false; capsLock = false; symbolsMode = false
        // Translit only makes sense in EN input mode (typing Latin → Ukrainian)
        if translitMode && currentLanguage != .en { translitMode = false }
        updateLayout()
    }

    private func handleTranslitToggle() {
        flushTranslitBuffer(force: true)
        translitMode.toggle()
        if translitMode { currentLanguage = .en }
        updateLayout()
    }

    private func handleSymbolsToggle() {
        flushTranslitBuffer(force: true)
        symbolsMode.toggle()
        updateLayout()
    }
}

// MARK: - ClaviKeyboardViewDelegate

extension KeyboardViewController: ClaviKeyboardViewDelegate {

    func didTapKey(_ key: KeyDef) {
        handleKey(key)
    }

    func didTapClip(at index: Int) {
        guard let text = clipboardHistory.fullText(at: index) else { return }
        textDocumentProxy.insertText(text)
    }

    func didLongPressClip(at index: Int) {
        clipboardHistory.remove(at: index)
    }

    func didTapClearClips() {
        clipboardHistory.clear()
    }

    func didTapDiacritic(_ variant: String) {
        // Delete base letter and insert diacritic variant
        textDocumentProxy.deleteBackward()
        textDocumentProxy.insertText(variant)
        keyboardView.diacriticItems = []
    }

    func didTapFix(_ fix: TextFixEngine.Fix) {
        // Replace text before cursor: delete original, insert fixed
        let deleteCount = fix.original.count
        for _ in 0..<deleteCount {
            textDocumentProxy.deleteBackward()
        }
        textDocumentProxy.insertText(fix.fixed)
        keyboardView.fixSuggestion = nil
    }

    func didDismissFix() {
        keyboardView.fixSuggestion = nil
    }

    func didTapTranslation(_ suggestion: TranslationEngine.TranslationSuggestion) {
        // Delete the original phrase and insert the translation
        for _ in 0..<suggestion.original.count {
            textDocumentProxy.deleteBackward()
        }
        textDocumentProxy.insertText(suggestion.translated)
        keyboardView.translationSuggestion = nil
    }

    func didDismissTranslation() {
        keyboardView.translationSuggestion = nil
    }

    func didRequestEmojiPanel() {
        let panel = EmojiPanel()
        panel.translatesAutoresizingMaskIntoConstraints = false
        panel.onEmojiSelected = { [weak self, weak panel] emoji in
            self?.textDocumentProxy.insertText(emoji)
            panel?.removeFromSuperview()
        }
        panel.onDismiss = { [weak panel] in panel?.removeFromSuperview() }
        view.addSubview(panel)
        NSLayoutConstraint.activate([
            panel.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            panel.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            panel.topAnchor.constraint(equalTo: view.topAnchor),
            panel.bottomAnchor.constraint(equalTo: view.bottomAnchor),
        ])
    }

    func didTapPrediction(_ word: String) {
        textDocumentProxy.insertText(word + " ")
        // Re-predict based on word just tapped
        let before = (textDocumentProxy.documentContextBeforeInput ?? "") + word + " "
        keyboardView.predictionItems = predictionEngine.predict(before, language: currentLanguage)
        if shifted && !capsLock { shifted = false; updateLayout() }
    }

    func didTapAutocorrectReject(original: String) {
        guard let ac = lastAutocorrect else { return }
        // Delete replacement + trailing space, restore original + space
        for _ in 0..<(ac.replaced.count + 1) { textDocumentProxy.deleteBackward() }
        textDocumentProxy.insertText(ac.original + " ")
        lastAutocorrect = nil
        keyboardView.autocorrectReject = nil
    }

    func didDismissAutocorrect() {
        lastAutocorrect = nil
        keyboardView.autocorrectReject = nil
    }

    func didTapNextKeyboard() {
        advanceToNextInputMode()
    }
}

// MARK: - Spell helpers

extension KeyboardViewController {

    private func spellEngine(for lang: Language) -> SpellEngine? {
        switch lang {
        case .en: return spellEngineEn
        case .uk: return spellEngineUk
        default:  return nil
        }
    }

    /// Returns the word currently being typed (no trailing space).
    private func extractCurrentWord(_ text: String) -> String? {
        let trimmed = text.trimmingCharacters(in: .init(charactersIn: " \n"))
        if trimmed.isEmpty { return nil }
        let parts = trimmed.components(separatedBy: .init(charactersIn: " \n"))
        return parts.last?.isEmpty == false ? parts.last : nil
    }

    /// Returns the last completed word (before the trailing space/newline we just committed).
    private func extractLastCompletedWord(_ textBefore: String) -> String? {
        let trimmed = textBefore.trimmingCharacters(in: .init(charactersIn: " \n"))
        if trimmed.isEmpty { return nil }
        let parts = trimmed.components(separatedBy: .init(charactersIn: " \n"))
        return parts.last?.isEmpty == false ? parts.last : nil
    }

    /// Returns false for words that should never be auto-corrected.
    private func shouldAutocorrect(_ word: String) -> Bool {
        guard word.count >= 3 else { return false }
        guard let first = word.first, !first.isUppercase else { return false }
        if word.contains(".") || word.contains("/") || word.contains(":") || word.contains("@") { return false }
        return true
    }
}
