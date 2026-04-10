package com.clavi.keyboard

import android.inputmethodservice.InputMethodService
import android.os.Handler
import android.os.Looper
import android.text.InputType
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

    // Language rotation: only cycle through languages the user enabled in Settings
    private var activeLanguages: List<Language> = listOf(Language.UK, Language.EN)
    private var currentLangIndex: Int = 0

    // Diacritics: userLocale = value from Settings; diacriticsLocale = current effective value
    // (may be auto-set when switching to a European language)
    private var userDiacriticsLocale: String? = null
    private var diacriticsLocale: String? = null

    // Translation engine — null when translation is disabled in settings
    private var translationEngine: TranslationEngine? = null

    private val translitBuffer = StringBuilder()

    // ── Lifecycle ──

    override fun onCreateInputView(): View {
        clipboardHistory = ClipboardHistory(this)
        clipboardHistory.listener = this

        // Load preferences
        val prefs = getSharedPreferences(SettingsActivity.PREFS_NAME, MODE_PRIVATE)
        userDiacriticsLocale = prefs.getString(SettingsActivity.PREF_DIACRITICS_LOCALE, null)
        diacriticsLocale = userDiacriticsLocale
        val savedLang = prefs.getString(SettingsActivity.PREF_DEFAULT_LANGUAGE, Language.UK.name)
        currentLanguage = Language.entries.firstOrNull { it.name == savedLang } ?: Language.UK
        keyboardView.hapticEnabled = prefs.getBoolean(SettingsActivity.PREF_HAPTIC, true)
        activeLanguages = SettingsActivity.loadActiveLanguages(prefs)
        currentLangIndex = activeLanguages.indexOf(currentLanguage).coerceAtLeast(0)
        val transSrc = prefs.getString(SettingsActivity.PREF_TRANSLATION_SOURCE, null)
        val transTgt = prefs.getString(SettingsActivity.PREF_TRANSLATION_TARGET, null)
        translationEngine = if (transSrc != null && transTgt != null)
            TranslationEngine(transSrc, transTgt, this) else null

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

        // Password field detection: hide clipboard strip and disable translit for privacy
        val inputType = info?.inputType ?: 0
        val variation = inputType and InputType.TYPE_MASK_VARIATION
        val isPassword = (inputType and InputType.TYPE_MASK_CLASS) == InputType.TYPE_CLASS_TEXT &&
            (variation == InputType.TYPE_TEXT_VARIATION_PASSWORD ||
             variation == InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD ||
             variation == InputType.TYPE_TEXT_VARIATION_WEB_PASSWORD)
        if (isPassword) {
            keyboardView.clipItems = emptyList()
            keyboardView.translationSuggestion = null
            if (translitMode) handleTranslitToggle()
        } else {
            // Refresh strip in case clipboard changed while keyboard was hidden
            keyboardView.clipItems = clipboardHistory.getRecent()
                .map { clipboardHistory.displayLabel(it) }
        }
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

    override fun onTranslationTap(suggestion: TranslationEngine.TranslationSuggestion) {
        val ic = currentInputConnection ?: return
        ic.deleteSurroundingText(suggestion.original.length, 0)
        ic.commitText(suggestion.translated, 1)
        keyboardView.translationSuggestion = null
    }

    override fun onTranslationDismiss() {
        keyboardView.translationSuggestion = null
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
            // After space or newline: run fix + translation analysis
            if (text == " " || text == "\n") {
                val textBefore = ic.getTextBeforeCursor(200, 0)?.toString() ?: ""
                val fix = TextFixEngine.analyze(textBefore)
                keyboardView.fixSuggestion = fix
                // Translation only when fix strip is not showing
                if (fix == null) {
                    keyboardView.translationSuggestion = null
                    translationEngine?.translate(textBefore) { suggestion ->
                        Handler(Looper.getMainLooper()).post {
                            keyboardView.translationSuggestion = suggestion
                        }
                    }
                } else {
                    keyboardView.translationSuggestion = null
                }
            } else {
                keyboardView.fixSuggestion = null
                keyboardView.translationSuggestion = null
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
        userDiacriticsLocale = locale
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
        keyboardView.translationSuggestion = null
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
        currentLangIndex = (currentLangIndex + 1) % activeLanguages.size
        currentLanguage = activeLanguages[currentLangIndex]

        // Auto-enable the appropriate diacritics locale for European languages,
        // restore user's Setting when switching back to UK/EN/QUC
        diacriticsLocale = currentLanguage.diacriticsLocale ?: userDiacriticsLocale

        shifted = false; capsLock = false; symbolsMode = false
        // Translit only applies when typing Latin to get Ukrainian (UK target, EN source)
        if (translitMode && currentLanguage != Language.EN) {
            translitMode = false
        }
        updateKeyboardLayout()
    }

    private fun handleTranslitToggle() {
        if (currentLanguage == Language.QUC) return  // translit not applicable for K'iche'
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
