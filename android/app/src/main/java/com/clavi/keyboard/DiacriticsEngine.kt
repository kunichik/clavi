package com.clavi.keyboard

/**
 * Smart diacritics engine for v0.3.
 *
 * When a user types a base letter in a language that uses diacritics,
 * this engine returns the most common diacritic variants (frequency-ordered,
 * most common first) to show in the suggestion strip above the keyboard.
 *
 * Supported languages: pt, pt_BR, de, no, fr, es, es_GT
 *
 * Usage:
 *   val suggestions = engine.suggest('a', "pt")
 *   // → ["ã", "â", "á", "à", "ä"] (most common first for Portuguese)
 */
object DiacriticsEngine {

    /**
     * Returns diacritic variants for a base letter in the given locale.
     * Returns empty list if no diacritics apply.
     * Always includes the base letter last so user can confirm without diacritic.
     */
    fun suggest(baseLetter: Char, locale: String): List<String> {
        val lang = locale.lowercase().split("_", "-").first()
        val table = tables[lang] ?: return emptyList()
        val lower = baseLetter.lowercaseChar()
        val variants = table[lower] ?: return emptyList()
        // If original was uppercase, capitalize all variants
        return if (baseLetter.isUpperCase()) variants.map { it.uppercase() }
               else variants
    }

    fun hasVariants(baseLetter: Char, locale: String): Boolean {
        val lang = locale.lowercase().split("_", "-").first()
        val table = tables[lang] ?: return false
        return table.containsKey(baseLetter.lowercaseChar())
    }

    // ── Diacritic tables ──
    // Variants are ordered by corpus frequency (most common first).
    // Source: Wiktionary frequency data + common word lists per language.

    private val tables: Map<String, Map<Char, List<String>>> = mapOf(

        // ── Portuguese (pt, pt_BR) ──
        // Heavy diacritic usage: ã â á à é ê í ó ô õ ú ç
        "pt" to mapOf(
            'a' to listOf("ã", "â", "á", "à", "a"),
            'e' to listOf("é", "ê", "e"),
            'i' to listOf("í", "i"),
            'o' to listOf("ô", "ó", "õ", "o"),
            'u' to listOf("ú", "ü", "u"),
            'c' to listOf("ç", "c"),
            'n' to listOf("ñ", "n"),
        ),

        // ── German (de) ──
        // ä ö ü ß — each quite frequent in everyday text
        "de" to mapOf(
            'a' to listOf("ä", "a"),
            'o' to listOf("ö", "o"),
            'u' to listOf("ü", "u"),
            's' to listOf("ß", "s"),
        ),

        // ── Norwegian (no, nb, nn) ──
        // æ ø å — always needed, very frequent
        "no" to mapOf(
            'a' to listOf("å", "a"),
            'e' to listOf("æ", "e"),     // æ written as 'e' phonetically sometimes
            'o' to listOf("ø", "o"),
        ),

        // Same for bokmål / nynorsk
        "nb" to mapOf(
            'a' to listOf("å", "a"),
            'e' to listOf("æ", "e"),
            'o' to listOf("ø", "o"),
        ),

        // ── French (fr) ──
        // é è ê ë à â ç î ï ô ù û
        "fr" to mapOf(
            'e' to listOf("é", "è", "ê", "ë", "e"),
            'a' to listOf("à", "â", "a"),
            'c' to listOf("ç", "c"),
            'i' to listOf("î", "ï", "i"),
            'o' to listOf("ô", "o"),
            'u' to listOf("ù", "û", "ü", "u"),
        ),

        // ── Spanish (es, es_GT) ──
        // ñ á é í ó ú ü ¡ ¿
        "es" to mapOf(
            'n' to listOf("ñ", "n"),
            'a' to listOf("á", "a"),
            'e' to listOf("é", "e"),
            'i' to listOf("í", "i"),
            'o' to listOf("ó", "o"),
            'u' to listOf("ú", "ü", "u"),
        ),

        // ── Swedish (sv) ──
        "sv" to mapOf(
            'a' to listOf("å", "ä", "a"),
            'o' to listOf("ö", "o"),
            'u' to listOf("ü", "u"),
        ),

        // ── Finnish (fi) ──
        "fi" to mapOf(
            'a' to listOf("ä", "a"),
            'o' to listOf("ö", "o"),
        ),

        // ── Polish (pl) ──
        "pl" to mapOf(
            'a' to listOf("ą", "a"),
            'c' to listOf("ć", "c"),
            'e' to listOf("ę", "e"),
            'l' to listOf("ł", "l"),
            'n' to listOf("ń", "n"),
            'o' to listOf("ó", "o"),
            's' to listOf("ś", "s"),
            'z' to listOf("ź", "ż", "z"),
        ),
    )
}
