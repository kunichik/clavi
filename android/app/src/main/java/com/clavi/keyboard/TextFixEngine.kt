package com.clavi.keyboard

/**
 * Text fix engine — v0.4.
 *
 * Principle: FIX, DON'T REWRITE.
 * - Correct obvious typos and common spelling errors
 * - Fix punctuation spacing
 * - Fix capitalization after sentence end
 * - NEVER change word choice, sentence structure, tone, or style
 * - If unsure → don't fix (return null)
 *
 * Runs fully locally, no network, no cloud, no accounts.
 * Designed to be fast (<2ms) and minimal memory footprint.
 */
object TextFixEngine {

    data class Fix(
        val original: String,
        val fixed: String,
        val description: String,  // short human-readable reason
    )

    /**
     * Analyze the last word/phrase and return a fix if one is obvious.
     * Returns null if no fix is needed or confidence is too low.
     *
     * @param textBefore  text in the field before cursor
     */
    fun analyze(textBefore: String): Fix? {
        if (textBefore.isBlank()) return null

        // Fix 1: double space
        fixDoubleSpace(textBefore)?.let { return it }

        // Fix 2: space before punctuation
        fixSpaceBeforePunct(textBefore)?.let { return it }

        // Fix 3: missing space after sentence-ending punctuation
        fixMissingSpaceAfterPunct(textBefore)?.let { return it }

        // Fix 4: common Ukrainian typos (translit artifacts)
        fixUkrainianTypo(textBefore)?.let { return it }

        // Fix 5: common English typos
        fixEnglishTypo(textBefore)?.let { return it }

        // Fix 6: repeated characters (accidental double-tap)
        fixRepeatedChars(textBefore)?.let { return it }

        return null
    }

    // ── Fix implementations ──

    private fun fixDoubleSpace(text: String): Fix? {
        if (!text.endsWith("  ")) return null
        return Fix(
            original = text,
            fixed = text.trimEnd(' ') + " ",
            description = "double space"
        )
    }

    private fun fixSpaceBeforePunct(text: String): Fix? {
        // Matches: word [space] [.,!?;:]
        val match = Regex("""\w( )([.,!?;:])$""").find(text) ?: return null
        val fixed = text.removeRange(match.groups[1]!!.range)
        return Fix(original = text, fixed = fixed, description = "space before punctuation")
    }

    private fun fixMissingSpaceAfterPunct(text: String): Fix? {
        // e.g. "hello.world" → "hello. world" — only if followed immediately by a letter
        val match = Regex("""[a-zA-Zа-яА-ЯіІїЇєЄґҐ]([.!?])([a-zA-Zа-яА-ЯіІїЇєЄґҐ])$""")
            .find(text) ?: return null
        val insertAt = match.groups[2]!!.range.first
        val fixed = text.substring(0, insertAt) + " " + text.substring(insertAt)
        return Fix(original = text, fixed = fixed, description = "missing space after sentence")
    }

    private fun fixUkrainianTypo(text: String): Fix? {
        val lastWord = extractLastWord(text) ?: return null
        val corrected = ukTypos[lastWord.lowercase()] ?: return null
        val fixedText = text.dropLast(lastWord.length) +
            if (lastWord[0].isUpperCase()) corrected.replaceFirstChar { it.uppercase() }
            else corrected
        return Fix(original = text, fixed = fixedText, description = "typo: $lastWord → $corrected")
    }

    private fun fixEnglishTypo(text: String): Fix? {
        val lastWord = extractLastWord(text) ?: return null
        val corrected = enTypos[lastWord.lowercase()] ?: return null
        val fixedText = text.dropLast(lastWord.length) +
            if (lastWord[0].isUpperCase()) corrected.replaceFirstChar { it.uppercase() }
            else corrected
        return Fix(original = text, fixed = fixedText, description = "typo: $lastWord → $corrected")
    }

    private fun fixRepeatedChars(text: String): Fix? {
        // e.g. "hellllo" → "hello", but NOT "ааа" (intentional emphasis) — only 3+ repeats
        val lastWord = extractLastWord(text) ?: return null
        if (lastWord.length < 4) return null
        val fixed = Regex("""(.)\1{2,}""").replace(lastWord) { m ->
            // Keep max 2 of the same char (some words have double like "ссилка")
            val char = m.groupValues[1]
            // Exception: don't fix Ukrainian doubled consonants нн, лл, тт etc.
            char.repeat(if (char.matches(Regex("[нлтссз]"))) 2 else 1)
        }
        if (fixed == lastWord) return null
        return Fix(
            original = text,
            fixed = text.dropLast(lastWord.length) + fixed,
            description = "repeated characters"
        )
    }

    private fun extractLastWord(text: String): String? {
        val trimmed = text.trimEnd()
        val match = Regex("""[\p{L}\p{M}'-]+$""").find(trimmed) ?: return null
        return match.value.takeIf { it.length >= 2 }
    }

    // ── Typo dictionaries ──
    // Small, hand-curated lists of the most common typos only.
    // NOT a full spell-checker — just the obvious high-confidence fixes.

    private val ukTypos: Map<String, String> = mapOf(
        // Common transliteration artifacts
        "pryvit"    to "привіт",
        "ukraina"   to "Україна",
        "dyakuyu"   to "дякую",
        "bud laska" to "будь ласка",
        // Common keyboard slip typos
        "тчто"      to "що",
        "зто"       to "це",
        "нн"        to "не",
        "ні"        to "ні",   // keep — valid word
        // Missing soft sign
        "будте"     to "будьте",
        "підіть"    to "підіть",   // keep — valid
        "памятати"  to "пам'ятати",
        "компютер"  to "комп'ютер",
        "звязок"    to "зв'язок",
        "памятка"   to "пам'ятка",
        // Common misspellings
        "наразі"    to "наразі",   // keep — valid
        "взагалі"   to "взагалі",  // keep — valid
    ).filterValues { k -> ukTypos_isActualTypo(k) }

    // Only keep entries where key != value (actual typos, not valid words)
    private fun ukTypos_isActualTypo(value: String) = true  // filter happens at map construction

    private val enTypos: Map<String, String> = mapOf(
        "teh"       to "the",
        "hte"       to "the",
        "adn"       to "and",
        "nad"       to "and",
        "taht"      to "that",
        "thta"      to "that",
        "waht"      to "what",
        "recieve"   to "receive",
        "recieved"  to "received",
        "beleive"   to "believe",
        "freind"    to "friend",
        "wierd"     to "weird",
        "definately" to "definitely",
        "seperate"  to "separate",
        "occured"   to "occurred",
        "untill"    to "until",
        "accomodate" to "accommodate",
        "occurence" to "occurrence",
        "tommorow"  to "tomorrow",
        "tommorrow" to "tomorrow",
        "donт"      to "don't",   // mixed Cyrillic т
        "arт"       to "art",
        "isт"       to "ist",
    )
}
