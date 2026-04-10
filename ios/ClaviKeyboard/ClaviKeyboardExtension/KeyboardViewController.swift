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

    private var diacriticsLocale: String? = nil   // set from app group UserDefaults

    // MARK: - Lifecycle

    override func viewDidLoad() {
        super.viewDidLoad()
        loadPreferences()
        setupKeyboard()
        clipboardHistory.onChange = { [weak self] in
            self?.keyboardView.clipItems = self?.clipboardHistory.recent ?? []
        }
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        loadPreferences()
        clipboardHistory.refresh()
        keyboardView.clipItems = clipboardHistory.recent
    }

    // MARK: - Preferences

    private func loadPreferences() {
        // Shared UserDefaults between app and extension (requires App Group entitlement)
        // Falls back to standard UserDefaults for now
        let defaults = UserDefaults(suiteName: "group.com.clavi.keyboard") ?? .standard
        diacriticsLocale = defaults.string(forKey: "diacritics_locale")
        let savedLang = defaults.string(forKey: "default_language") ?? "UK"
        currentLanguage = savedLang == "EN" ? .en : .uk
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
        } else {
            textDocumentProxy.insertText(text)
            showDiacriticsIfNeeded(text)

            if text == " " {
                let before = textDocumentProxy.documentContextBeforeInput ?? ""
                keyboardView.fixSuggestion = TextFixEngine.analyze(before)
            } else {
                keyboardView.fixSuggestion = nil
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
        currentLanguage = currentLanguage == .uk ? .en : .uk
        shifted = false; capsLock = false; symbolsMode = false
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

    func didTapNextKeyboard() {
        advanceToNextInputMode()
    }
}
