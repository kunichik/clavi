import UIKit

/**
 * Clavi iOS Custom Keyboard Extension
 *
 * Features (v0.5):
 * - Ukrainian ЙЦУКЕН + English QWERTY layouts
 * - Translit mode (KMU 2010): type Latin → get Ukrainian Cyrillic
 * - Language switch (UK ↔ EN)
 * - Clipboard history strip above keyboard
 * - Smart diacritics strip (v0.6)
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

    // MARK: - Lifecycle

    override func viewDidLoad() {
        super.viewDidLoad()
        setupKeyboard()
        clipboardHistory.onChange = { [weak self] in
            self?.keyboardView.clipItems = self?.clipboardHistory.recent ?? []
        }
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        // Refresh clipboard when keyboard appears
        clipboardHistory.refresh()
        keyboardView.clipItems = clipboardHistory.recent
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
        let rows: [[KeyDef]]
        if symbolsMode {
            rows = KeyboardLayout.symbols
        } else {
            rows = KeyboardLayout.layout(for: currentLanguage, shifted: shifted)
        }
        keyboardView.rows = rows
        keyboardView.translitActive = translitMode
        keyboardView.currentLanguage = currentLanguage
    }

    // MARK: - Key handling

    private func handleKey(_ key: KeyDef) {
        switch key.action {
        case .character(let ch):
            handleCharacter(ch)
        case .shift:
            handleShift()
        case .backspace:
            handleBackspace()
        case .langSwitch:
            handleLangSwitch()
        case .translit:
            handleTranslitToggle()
        case .symbols:
            handleSymbolsToggle()
        case .enter:
            handleEnter()
        case .space:
            handleCharacter(" ")
        }
    }

    private func handleCharacter(_ text: String) {
        if translitMode && currentLanguage == .en {
            translitBuffer += text
            flushTranslitBuffer(force: false)
        } else {
            textDocumentProxy.insertText(text)
        }
        if shifted && !capsLock {
            shifted = false
            updateLayout()
        }
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
        shifted = false
        capsLock = false
        symbolsMode = false
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

// MARK: - ClaviKeyboardView delegate

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

    func didTapNextKeyboard() {
        advanceToNextInputMode()
    }
}
