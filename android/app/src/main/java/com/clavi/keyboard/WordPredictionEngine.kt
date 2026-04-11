package com.clavi.keyboard

import android.content.Context
import java.io.IOException

/**
 * Word prediction engine (v1.5).
 *
 * Loads a per-language bigram TSV from assets (predict/{lang}.tsv):
 *   trigger_word<TAB>suggestion1<TAB>suggestion2<TAB>suggestion3
 *
 * Empty trigger (line starting with TAB) = default suggestions for start-of-sentence.
 *
 * Supports EN and UK; other languages silently return empty list.
 *
 * Usage:
 *   engine.predict("Hello how", Language.EN)  // → ["are", "much", "do"]
 *   engine.predict("", Language.UK)            // → ["Я", "Це", "Привіт"]
 */
class WordPredictionEngine(private val context: Context) {

    // Cache: lang key → bigram map
    private val cache = mutableMapOf<String, Map<String, List<String>>>()

    /**
     * Returns up to [maxResults] predicted next words given [textBefore] context.
     * Returns empty list if language has no prediction data or context gives no match.
     */
    fun predict(textBefore: String, language: Language, maxResults: Int = 3): List<String> {
        val assetKey = assetKey(language) ?: return emptyList()
        val dict = cache.getOrPut(assetKey) { loadDict(assetKey) }
        if (dict.isEmpty()) return emptyList()

        val lastWord = extractLastWord(textBefore).lowercase()

        // Exact match → suggestions for this trigger
        val exact = dict[lastWord]
        if (!exact.isNullOrEmpty()) return exact.take(maxResults)

        // No match for specific word → fall back to default context
        return (dict[""] ?: emptyList()).take(maxResults)
    }

    private fun assetKey(language: Language): String? = when (language) {
        Language.EN -> "en"
        Language.UK -> "uk"
        else        -> null   // no predictions for DE/FR/ES/etc. yet
    }

    private fun extractLastWord(text: String): String {
        val trimmed = text.trimEnd()
        if (trimmed.isEmpty()) return ""
        return trimmed.substringAfterLast(' ').substringAfterLast('\n')
    }

    private fun loadDict(lang: String): Map<String, List<String>> {
        return try {
            context.assets.open("predict/$lang.tsv")
                .bufferedReader()
                .lineSequence()
                .filter { it.isNotBlank() && !it.startsWith('#') }
                .mapNotNull { line ->
                    val parts = line.split('\t')
                    if (parts.isEmpty()) return@mapNotNull null
                    val trigger = parts[0]
                    val suggestions = parts.drop(1).filter { it.isNotBlank() }
                    if (suggestions.isEmpty()) return@mapNotNull null
                    trigger to suggestions
                }
                .toMap()
        } catch (_: IOException) {
            emptyMap()
        }
    }
}
