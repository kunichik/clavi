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

        // Fix 7: accidental caps (e.g. "ПРивіт" → "Привіт")
        fixAccidentalCaps(textBefore)?.let { return it }

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

    private fun fixAccidentalCaps(text: String): Fix? {
        val word = extractLastWord(text) ?: return null
        if (word.length < 4) return null
        // Pattern: first two chars uppercase, at least one lowercase after (e.g. "ПРивіт", "HEllo")
        if (!word[0].isUpperCase() || !word[1].isUpperCase()) return null
        if (word.drop(2).none { it.isLowerCase() }) return null
        // Fix: keep first char uppercase, lowercase the rest
        val fixed = word[0].toString() + word.drop(1).lowercase()
        if (fixed == word) return null
        return Fix(
            original = text,
            fixed = text.dropLast(word.length) + fixed,
            description = "caps: ${word.take(6)}… → ${fixed.take(6)}…"
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
        // Transliteration artifacts (Latin → Cyrillic)
        "pryvit"    to "привіт",
        "dyakuyu"   to "дякую",
        "prosto"    to "просто",
        "dobre"     to "добре",
        "tak"       to "так",
        "shcho"     to "що",
        "yakshcho"  to "якщо",
        "tomu"      to "тому",
        "potim"     to "потім",
        "znayu"     to "знаю",
        "rozumiyu"  to "розумію",
        "bud"       to "буд",  // ambiguous — skip; keep for explicit "bud laska"
        // Missing apostrophe (most common Ukrainian typo category)
        "памятати"  to "пам'ятати",
        "памятка"   to "пам'ятка",
        "памяти"    to "пам'яті",
        "память"    to "пам'ять",
        "компютер"  to "комп'ютер",
        "звязок"    to "зв'язок",
        "звязати"   to "зв'язати",
        "мяч"       to "м'яч",
        "обєкт"     to "об'єкт",
        "субєкт"    to "суб'єкт",
        "девять"    to "дев'ять",
        "пять"      to "п'ять",
        "вязати"    to "в'язати",
        "вязаний"   to "в'язаний",
        "єднати"    to "з'єднати",
        "зєднати"   to "з'єднати",
        // Missing soft sign
        "будте"     to "будьте",
        "прийдіть"  to "прийдіть",  // valid — keep
        // Common keyboard slips (Cyrillic)
        "тчто"      to "що",
        "зто"       to "це",
        // Mixed Cyrillic/Latin
        "donт"      to "don't",
    )

    private val enTypos: Map<String, String> = mapOf(
        // Classic finger-roll transpositions
        "teh"         to "the",
        "hte"         to "the",
        "adn"         to "and",
        "nad"         to "and",
        "taht"        to "that",
        "thta"        to "that",
        "waht"        to "what",
        "hwat"        to "what",
        "yuo"         to "you",
        "youre"       to "you're",
        "dont"        to "don't",
        "doesnt"      to "doesn't",
        "didnt"       to "didn't",
        "wont"        to "won't",
        "cant"        to "can't",
        "isnt"        to "isn't",
        "wasnt"       to "wasn't",
        "werent"      to "weren't",
        "havent"      to "haven't",
        "hasnt"       to "hasn't",
        "wouldnt"     to "wouldn't",
        "couldnt"     to "couldn't",
        "shouldnt"    to "shouldn't",
        "im"          to "I'm",
        "ive"         to "I've",
        "id"          to "I'd",
        "ill"         to "I'll",
        // i-before-e violations
        "recieve"     to "receive",
        "recieved"    to "received",
        "beleive"     to "believe",
        "beleived"    to "believed",
        "freind"      to "friend",
        "wierd"       to "weird",
        "acheive"     to "achieve",
        "acheived"    to "achieved",
        "peice"       to "piece",
        "releive"     to "relieve",
        // Double-letter confusion
        "occured"     to "occurred",
        "occurence"   to "occurrence",
        "accomodate"  to "accommodate",
        "embarass"    to "embarrass",
        "harrass"     to "harass",
        "necesary"    to "necessary",
        "posession"   to "possession",
        "agressive"   to "aggressive",
        "profesional" to "professional",
        "recomend"    to "recommend",
        // -ately / -itely
        "definately"  to "definitely",
        "definetly"   to "definitely",
        "absolutley"  to "absolutely",
        "immediatley" to "immediately",
        // -ate / -ete confusion
        "seperate"    to "separate",
        "seperated"   to "separated",
        "desparate"   to "desperate",
        // Common misspellings
        "untill"      to "until",
        "tommorow"    to "tomorrow",
        "tommorrow"   to "tomorrow",
        "calender"    to "calendar",
        "cemetary"    to "cemetery",
        "goverment"   to "government",
        "medecine"    to "medicine",
        "millenium"   to "millennium",
        "miniscule"   to "minuscule",
        "mischevous"  to "mischievous",
        "neccessary"  to "necessary",
        "noticable"   to "noticeable",
        "priviledge"  to "privilege",
        "pronounciation" to "pronunciation",
        "publically"  to "publicly",
        "questionaire" to "questionnaire",
        "succesful"   to "successful",
        "suprise"     to "surprise",
        "tendancy"    to "tendency",
        "truely"      to "truly",
        "useable"     to "usable",
        "vaccum"      to "vacuum",
        "withold"     to "withhold",
        // Mixed Cyrillic/Latin keys (common on bilingual keyboards)
        "donт"        to "don't",
        "arт"         to "art",
        "isт"         to "ist",
    )
}
