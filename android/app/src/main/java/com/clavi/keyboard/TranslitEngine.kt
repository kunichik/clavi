package com.clavi.keyboard

/**
 * KMU 2010 Ukrainian transliteration engine.
 * Latin (phonetic) -> Ukrainian Cyrillic.
 * Longest-match-first strategy for digraphs/trigraphs.
 */
class TranslitEngine {

    private data class Rule(val latin: String, val cyrillic: String)

    // Sorted by length descending so longest match wins
    private val rules: List<Rule> = listOf(
        // Trigraphs
        Rule("shch", "\u0449"), Rule("Shch", "\u0429"), Rule("SHCH", "\u0429"),
        // Digraphs
        Rule("zh", "\u0436"), Rule("Zh", "\u0416"), Rule("ZH", "\u0416"),
        Rule("kh", "\u0445"), Rule("Kh", "\u0425"), Rule("KH", "\u0425"),
        Rule("ts", "\u0446"), Rule("Ts", "\u0426"), Rule("TS", "\u0426"),
        Rule("ch", "\u0447"), Rule("Ch", "\u0427"), Rule("CH", "\u0427"),
        Rule("sh", "\u0448"), Rule("Sh", "\u0428"), Rule("SH", "\u0428"),
        // yu/ya — user-friendly variants (type ya→я, yu→ю)
        Rule("yu", "\u044E"), Rule("Yu", "\u042E"), Rule("YU", "\u042E"),
        Rule("ya", "\u044F"), Rule("Ya", "\u042F"), Rule("YA", "\u042F"),
        Rule("ye", "\u0454"), Rule("Ye", "\u0404"), Rule("YE", "\u0404"),
        Rule("yi", "\u0457"), Rule("Yi", "\u0407"), Rule("YI", "\u0407"),
        // Legacy ia/iu variants
        Rule("iu", "\u044E"), Rule("Iu", "\u042E"), Rule("IU", "\u042E"),
        Rule("ia", "\u044F"), Rule("Ia", "\u042F"), Rule("IA", "\u042F"),
        Rule("ie", "\u0454"), Rule("Ie", "\u0404"), Rule("IE", "\u0404"),
        // Apostrophe
        Rule("'", "\u02BC"),
        // Single chars
        Rule("a", "\u0430"), Rule("A", "\u0410"),
        Rule("b", "\u0431"), Rule("B", "\u0411"),
        Rule("v", "\u0432"), Rule("V", "\u0412"),
        Rule("h", "\u0433"), Rule("H", "\u0413"),
        Rule("g", "\u0491"), Rule("G", "\u0490"),
        Rule("d", "\u0434"), Rule("D", "\u0414"),
        Rule("e", "\u0435"), Rule("E", "\u0415"),
        Rule("z", "\u0437"), Rule("Z", "\u0417"),
        Rule("y", "\u0438"), Rule("Y", "\u0418"),
        Rule("i", "\u0456"), Rule("I", "\u0406"),
        Rule("j", "\u0439"), Rule("J", "\u0419"),
        Rule("k", "\u043A"), Rule("K", "\u041A"),
        Rule("l", "\u043B"), Rule("L", "\u041B"),
        Rule("m", "\u043C"), Rule("M", "\u041C"),
        Rule("n", "\u043D"), Rule("N", "\u041D"),
        Rule("o", "\u043E"), Rule("O", "\u041E"),
        Rule("p", "\u043F"), Rule("P", "\u041F"),
        Rule("r", "\u0440"), Rule("R", "\u0420"),
        Rule("s", "\u0441"), Rule("S", "\u0421"),
        Rule("t", "\u0442"), Rule("T", "\u0422"),
        Rule("u", "\u0443"), Rule("U", "\u0423"),
        Rule("f", "\u0444"), Rule("F", "\u0424"),
        Rule("x", "\u043A\u0441"), Rule("X", "\u041A\u0421"),
        Rule("c", "\u0441"), Rule("C", "\u0421"),
        Rule("q", "\u043A"), Rule("Q", "\u041A"),
        Rule("w", "\u0432"), Rule("W", "\u0412"),
    )

    private val maxRuleLen = rules.maxOf { it.latin.length }

    /**
     * Transliterate a full string from Latin to Ukrainian Cyrillic.
     */
    fun transliterate(input: String): String {
        val result = StringBuilder(input.length)
        var i = 0
        while (i < input.length) {
            var matched = false
            // Try longest match first
            val maxLen = minOf(maxRuleLen, input.length - i)
            for (len in maxLen downTo 1) {
                val candidate = input.substring(i, i + len)
                val rule = rules.find { it.latin == candidate }
                if (rule != null) {
                    result.append(rule.cyrillic)
                    i += len
                    matched = true
                    break
                }
            }
            if (!matched) {
                result.append(input[i])
                i++
            }
        }
        return result.toString()
    }

    /**
     * Transliterate a single character (for real-time input).
     * Returns null if the char might be part of a digraph and we need to buffer.
     */
    fun isDigraphStart(c: Char): Boolean {
        val lower = c.lowercaseChar()
        return lower in setOf('s', 'z', 'k', 't', 'c', 'i', 'y')
    }

    /**
     * Try to transliterate a buffered sequence. Returns the result and how many
     * chars were consumed, or null if no match.
     */
    fun tryTransliterate(buffer: String): Pair<String, Int>? {
        for (len in minOf(maxRuleLen, buffer.length) downTo 1) {
            val candidate = buffer.substring(0, len)
            val rule = rules.find { it.latin == candidate }
            if (rule != null) {
                return Pair(rule.cyrillic, len)
            }
        }
        return null
    }
}
