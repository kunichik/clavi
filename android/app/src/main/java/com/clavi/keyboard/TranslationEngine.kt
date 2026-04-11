package com.clavi.keyboard

import android.content.Context
import com.google.android.gms.tasks.Task
import com.google.mlkit.common.model.DownloadConditions
import com.google.mlkit.nl.translate.TranslateLanguage
import com.google.mlkit.nl.translate.Translation
import com.google.mlkit.nl.translate.TranslatorOptions
import java.io.IOException

/**
 * Two-tier translation engine.
 *
 * Tier 1: Static TSV word dictionary bundled in assets/translate/{src}-{tgt}.tsv
 *         Zero download, word-level, instant.
 * Tier 2: ML Kit on-device translator (sentence-level, ~30MB model download on WiFi).
 *         Falls back to Tier 1 if model not yet downloaded or translation fails.
 *
 * Usage:
 *   val engine = TranslationEngine("en", "uk", context)
 *   engine.translate("Hello how are you") { suggestion -> ... }
 */
class TranslationEngine(
    val sourceLang: String,
    val targetLang: String,
    private val context: Context,
) {

    data class TranslationSuggestion(
        val original: String,    // phrase that was translated
        val translated: String,  // translation result
        val sourceLang: String,
        val targetLang: String,
    )

    // Tier 1: static TSV dictionary, lazy-loaded
    private val dictionary: Map<String, String> by lazy { loadDict() }

    // Tier 2: ML Kit on-device translator, lazy-created
    private val mlTranslator by lazy {
        val src = TranslateLanguage.fromLanguageTag(sourceLang) ?: TranslateLanguage.ENGLISH
        val tgt = TranslateLanguage.fromLanguageTag(targetLang) ?: TranslateLanguage.UKRAINIAN
        Translation.getClient(
            TranslatorOptions.Builder()
                .setSourceLanguage(src)
                .setTargetLanguage(tgt)
                .build()
        )
    }

    /**
     * Async translate. Tries ML Kit first; falls back to TSV word lookup on failure.
     * Calls [callback] on the calling thread (usually main thread via Task listeners).
     * Returns null if there's nothing worth translating in the given text.
     */
    fun translate(phraseBeforeCursor: String, callback: (TranslationSuggestion?) -> Unit) {
        val phrase = extractPhrase(phraseBeforeCursor) ?: return callback(null)
        mlTranslator.translate(phrase)
            .addOnSuccessListener { result ->
                callback(TranslationSuggestion(phrase, result, sourceLang, targetLang))
            }
            .addOnFailureListener {
                // Tier 1 fallback: look up the last word in the static dictionary
                val lastWord = phrase.trim().substringAfterLast(' ').ifEmpty { phrase.trim() }
                val result = dictionary[lastWord.lowercase()]
                callback(result?.let { TranslationSuggestion(lastWord, it, sourceLang, targetLang) })
            }
    }

    /**
     * Pre-fetch the ML Kit model over WiFi. Call from Settings "Download model" button.
     * [onDone] is called with true on success, false on failure.
     */
    fun downloadModelIfNeeded(onDone: (Boolean) -> Unit) {
        val conditions = DownloadConditions.Builder().requireWifi().build()
        mlTranslator.downloadModelIfNeeded(conditions)
            .addOnSuccessListener { onDone(true) }
            .addOnFailureListener { onDone(false) }
    }

    // ── Phrase extraction ──

    /**
     * Extract the phrase to translate from text before cursor.
     * Returns the text after the last sentence-ending punctuation (or the whole text),
     * trimmed. Returns null if too short to bother translating.
     */
    private fun extractPhrase(text: String): String? {
        val trimmed = text.trimEnd()
        if (trimmed.isEmpty()) return null
        val sentenceStart = trimmed.lastIndexOfAny(charArrayOf('.', '!', '?', '\n'))
        val phrase = if (sentenceStart >= 0) trimmed.substring(sentenceStart + 1).trim()
                     else trimmed
        return phrase.takeIf { it.length > 2 }
    }

    // ── TSV dictionary loader ──

    private fun loadDict(): Map<String, String> {
        return try {
            context.assets.open("translate/$sourceLang-$targetLang.tsv")
                .bufferedReader()
                .lineSequence()
                .filter { it.isNotBlank() && !it.startsWith('#') }
                .mapNotNull { line ->
                    val tab = line.indexOf('\t')
                    if (tab < 0) null else line.substring(0, tab) to line.substring(tab + 1)
                }
                .toMap()
        } catch (_: IOException) {
            emptyMap()
        }
    }
}
