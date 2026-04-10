package com.clavi.keyboard

import android.inputmethodservice.InputMethodService
import android.view.View
import android.view.inputmethod.EditorInfo
import android.widget.Toast

/**
 * Clavi Input Method Service.
 *
 * Features:
 * - Ukrainian ЙЦУКЕН + English QWERTY keyboard
 * - Translit mode (KMU 2010): type Latin → get Ukrainian
 * - Language switch (UK ↔ EN)
 * - Clipboard history strip above keyboard (v0.2)
 */
class ClaviIME : InputMethodService(),
    ClaviKeyboardView.OnKeyListener,
    ClaviKeyboardView.OnStripListener,
    ClipboardHistory.OnChangeListener {

    private lateinit var keyboardView: ClaviKeyboardView
    private val translitEngine = TranslitEngine()
    private lateinit var clipboardHistory: ClipboardHistory

    private var currentLanguage = Language.UK
    private var shifted = false
    private var capsLock = false
    private var translitMode = false
    private var symbolsMode = false

    // Active diacritics locale (null = diacritics off)
    // Set this to e.g. "pt", "de", "fr" to enable the smart diacritics strip
    private var diacriticsLocale: String? = null

    private val translitBuffer = StringBuilder()

    // ── Lifecycle ──

    override fun onCreateInputView(): View {
        clipboardHistory = ClipboardHistory(this)
        clipboardHistory.listener = this

        keyboardView = ClaviKeyboardView(this)
        keyboardView.listener = this
        keyboardView.stripListener = this
        updateKeyboardLayout()

        clipboardHistory.startListening()
        return keyboardView
    }

    override fun onStartInputView(info: EditorInfo?, restarting: Boolean) {
        super.onStartInputView(info, restarting)
        translitBuffer.clear()
        updateKeyboardLayout()
        // Refresh strip in case clipboard changed while keyboard was hidden
        keyboardView.clipItems = clipboardHistory.getRecent()
            .map { clipboardHistory.displayLabel(it) }
    }

    override fun onFinishInputView(finishingInput: Boolean) {
        super.onFinishInputView(finishingInput)
        translitBuffer.clear()
    }

    override fun onDestroy() {
        super.onDestroy()
        if (::clipboardHistory.isInitialized) clipboardHistory.stopListening()
    }

    // ── ClipboardHistory.OnChangeListener ──

    override fun onClipboardChanged(clips: List<String>) {
        if (::keyboardView.isInitialized) {
            keyboardView.clipItems = clips.map { clipboardHistory.displayLabel(it) }
        }
    }

    // ── ClaviKeyboardView.OnStripListener ──

    override fun onClipTap(index: Int) {
        val text = clipboardHistory.getFullText(index) ?: return
        currentInputConnection?.commitText(text, 1)
    }

    override fun onClipLongPress(index: Int) {
        clipboardHistory.removeAt(index)
    }

    override fun onStripClear() {
        clipboardHistory.clear()
    }

    override fun onDiacriticTap(variant: String) {
        val ic = currentInputConnection ?: return
        // Delete the last character (base letter) and replace with diacritic variant
        ic.deleteSurroundingText(1, 0)
        ic.commitText(variant, 1)
        clearDiacritics()
    }

    override fun onFixTap(fix: TextFixEngine.Fix) {
        val ic = currentInputConnection ?: return
        // Replace text before cursor: delete the original and insert the fixed version
        ic.deleteSurroundingText(fix.original.length, 0)
        ic.commitText(fix.fixed, 1)
        keyboardView.fixSuggestion = null
    }

    // ── ClaviKeyboardView.OnKeyListener ──

    override fun onKey(key: Key) {
        when (key.code) {
            KeyboardLayout.KEYCODE_SHIFT    -> handleShift()
            KeyboardLayout.KEYCODE_BACKSPACE -> handleBackspace()
            KeyboardLayout.KEYCODE_LANG_SWITCH -> handleLanguageSwitch()
            KeyboardLayout.KEYCODE_TRANSLIT -> handleTranslitToggle()
            KeyboardLayout.KEYCODE_ENTER    -> handleEnter()
            KeyboardLayout.KEYCODE_SYMBOLS  -> handleSymbolsToggle()
            else -> handleCharacter(key)
        }
    }

    // ── Key handlers ──

    private fun handleCharacter(key: Key) {
        val ic = currentInputConnection ?: return
        val text = key.label

        if (translitMode && currentLanguage == Language.EN) {
            translitBuffer.append(text)
            flushTranslitBuffer(force = false)
            clearDiacritics()
            keyboardView.fixSuggestion = null
        } else {
            ic.commitText(text, 1)
            // Show diacritics strip if this letter has variants in the active locale
            showDiacriticsIfNeeded(text)
            // After space: run fix analysis on the text before cursor
            if (text == " ") {
                val textBefore = ic.getTextBeforeCursor(200, 0)?.toString() ?: ""
                keyboardView.fixSuggestion = TextFixEngine.analyze(textBefore)
            } else {
                keyboardView.fixSuggestion = null
            }
        }

        if (shifted && !capsLock) {
            shifted = false
            updateKeyboardLayout()
        }
    }

    private fun showDiacriticsIfNeeded(text: String) {
        val locale = diacriticsLocale ?: return
        val char = text.singleOrNull() ?: run { clearDiacritics(); return }
        val variants = DiacriticsEngine.suggest(char, locale)
        if (variants.size > 1) {  // size=1 means only the base letter → skip
            keyboardView.diacriticItems = variants
        } else {
            clearDiacritics()
        }
    }

    private fun clearDiacritics() {
        if (::keyboardView.isInitialized) keyboardView.diacriticItems = emptyList()
    }

    /** Call this to enable/disable the diacritics strip. locale = "pt", "de", "fr", etc. or null to disable. */
    fun setDiacriticsLocale(locale: String?) {
        diacriticsLocale = locale
        clearDiacritics()
    }

    private fun flushTranslitBuffer(force: Boolean) {
        val ic = currentInputConnection ?: return
        while (translitBuffer.isNotEmpty()) {
            val result = translitEngine.tryTransliterate(translitBuffer.toString())
            if (result != null) {
                ic.commitText(result.first, 1)
                translitBuffer.delete(0, result.second)
            } else if (force || !translitEngine.isDigraphStart(translitBuffer[0])) {
                ic.commitText(translitBuffer[0].toString(), 1)
                translitBuffer.deleteCharAt(0)
            } else {
                break
            }
        }
    }

    private fun handleBackspace() {
        val ic = currentInputConnection ?: return
        clearDiacritics()
        keyboardView.fixSuggestion = null
        if (translitBuffer.isNotEmpty()) {
            translitBuffer.deleteCharAt(translitBuffer.length - 1)
        } else {
            ic.deleteSurroundingText(1, 0)
        }
    }

    private fun handleEnter() {
        flushTranslitBuffer(force = true)
        currentInputConnection?.commitText("\n", 1)
    }

    private fun handleShift() {
        if (shifted) {
            capsLock = !capsLock
            shifted = capsLock
        } else {
            shifted = true
            capsLock = false
        }
        updateKeyboardLayout()
    }

    private fun handleLanguageSwitch() {
        flushTranslitBuffer(force = true)
        currentLanguage = when (currentLanguage) {
            Language.UK -> Language.EN
            Language.EN -> Language.UK
        }
        shifted = false
        capsLock = false
        symbolsMode = false
        updateKeyboardLayout()
    }

    private fun handleTranslitToggle() {
        flushTranslitBuffer(force = true)
        translitMode = !translitMode
        if (translitMode) currentLanguage = Language.EN
        val msg = if (translitMode) getString(R.string.translit_on)
                  else getString(R.string.translit_off)
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
        updateKeyboardLayout()
    }

    private fun handleSymbolsToggle() {
        flushTranslitBuffer(force = true)
        symbolsMode = !symbolsMode
        updateKeyboardLayout()
    }

    private fun updateKeyboardLayout() {
        if (!::keyboardView.isInitialized) return
        val rows = if (symbolsMode) KeyboardLayout.getSymbolsLayout()
                   else KeyboardLayout.getLayout(currentLanguage, shifted)
        keyboardView.currentLanguage = currentLanguage
        keyboardView.translitActive = translitMode
        keyboardView.setLayout(rows)
    }
}
