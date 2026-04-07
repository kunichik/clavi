package com.clavi.keyboard

import android.inputmethodservice.InputMethodService
import android.view.View
import android.view.inputmethod.EditorInfo
import android.widget.Toast

/**
 * Clavi Input Method Service.
 *
 * Features:
 * - Ukrainian ЙЦУКЕН keyboard
 * - English QWERTY keyboard
 * - Translit mode (KMU 2010): type Latin -> get Ukrainian
 * - Language switch button
 */
class ClaviIME : InputMethodService(), ClaviKeyboardView.OnKeyListener {

    private lateinit var keyboardView: ClaviKeyboardView
    private val translitEngine = TranslitEngine()

    private var currentLanguage = Language.UK
    private var shifted = false
    private var capsLock = false
    private var translitMode = false
    private var symbolsMode = false

    // Buffer for translit digraph matching
    private val translitBuffer = StringBuilder()

    override fun onCreateInputView(): View {
        keyboardView = ClaviKeyboardView(this)
        keyboardView.listener = this
        updateKeyboardLayout()
        return keyboardView
    }

    override fun onStartInputView(info: EditorInfo?, restarting: Boolean) {
        super.onStartInputView(info, restarting)
        translitBuffer.clear()
        updateKeyboardLayout()
    }

    override fun onKey(key: Key) {
        when (key.code) {
            KeyboardLayout.KEYCODE_SHIFT -> handleShift()
            KeyboardLayout.KEYCODE_BACKSPACE -> handleBackspace()
            KeyboardLayout.KEYCODE_LANG_SWITCH -> handleLanguageSwitch()
            KeyboardLayout.KEYCODE_TRANSLIT -> handleTranslitToggle()
            KeyboardLayout.KEYCODE_ENTER -> handleEnter()
            KeyboardLayout.KEYCODE_SYMBOLS -> handleSymbolsToggle()
            else -> handleCharacter(key)
        }
    }

    private fun handleCharacter(key: Key) {
        val ic = currentInputConnection ?: return
        val text = key.label

        if (translitMode && currentLanguage == Language.EN) {
            // In translit mode on English layout: buffer chars for digraph matching
            translitBuffer.append(text)
            flushTranslitBuffer(force = false)
        } else {
            ic.commitText(text, 1)
        }

        // Auto-unshift after one character (unless caps lock)
        if (shifted && !capsLock) {
            shifted = false
            updateKeyboardLayout()
        }
    }

    private fun flushTranslitBuffer(force: Boolean) {
        val ic = currentInputConnection ?: return

        while (translitBuffer.isNotEmpty()) {
            val result = translitEngine.tryTransliterate(translitBuffer.toString())
            if (result != null) {
                ic.commitText(result.first, 1)
                translitBuffer.delete(0, result.second)
            } else if (force || !translitEngine.isDigraphStart(translitBuffer[0])) {
                // No match possible, output as-is
                ic.commitText(translitBuffer[0].toString(), 1)
                translitBuffer.deleteCharAt(0)
            } else {
                // Might be start of a digraph, wait for more input
                break
            }
        }
    }

    private fun handleBackspace() {
        val ic = currentInputConnection ?: return
        if (translitBuffer.isNotEmpty()) {
            translitBuffer.deleteCharAt(translitBuffer.length - 1)
        } else {
            ic.deleteSurroundingText(1, 0)
        }
    }

    private fun handleEnter() {
        flushTranslitBuffer(force = true)
        val ic = currentInputConnection ?: return
        ic.commitText("\n", 1)
    }

    private fun handleShift() {
        if (shifted) {
            // Double-tap shift = caps lock
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

        // Switch to English layout when translit is on (user types Latin)
        if (translitMode) {
            currentLanguage = Language.EN
        }

        val msg = if (translitMode) {
            getString(R.string.translit_on)
        } else {
            getString(R.string.translit_off)
        }
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

        val rows = if (symbolsMode) {
            KeyboardLayout.getSymbolsLayout()
        } else {
            KeyboardLayout.getLayout(currentLanguage, shifted)
        }

        keyboardView.currentLanguage = currentLanguage
        keyboardView.translitActive = translitMode
        keyboardView.setLayout(rows)
    }
}
