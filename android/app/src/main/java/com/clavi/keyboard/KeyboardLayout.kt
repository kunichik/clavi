package com.clavi.keyboard

enum class Language {
    UK, EN,
    DE, FR, ES, PT, NO, ES_GT,  // v0.5 — Western European
    QUC,                          // K'iche' Maya
}

/** BCP 47 diacritics locale to auto-enable when switching to this language. Null = none. */
val Language.diacriticsLocale: String? get() = when (this) {
    Language.DE    -> "de"
    Language.FR    -> "fr"
    Language.ES, Language.ES_GT -> "es"
    Language.PT    -> "pt"
    Language.NO    -> "no"
    else           -> null
}

/** Short label shown on the language-switch key. */
val Language.label: String get() = when (this) {
    Language.UK    -> "УК"
    Language.EN    -> "EN"
    Language.DE    -> "DE"
    Language.FR    -> "FR"
    Language.ES    -> "ES"
    Language.PT    -> "PT"
    Language.NO    -> "NO"
    Language.ES_GT -> "GT"
    Language.QUC   -> "Q'"
}

data class Key(
    val label: String,
    val code: Int = 0,           // Android keycode or Unicode codepoint
    val widthMultiplier: Float = 1f,
    val isSpecial: Boolean = false,
    val icon: String? = null,    // For special keys like shift, backspace
)

data class Row(val keys: List<Key>)

object KeyboardLayout {

    // Special key codes
    const val KEYCODE_SHIFT = -1
    const val KEYCODE_BACKSPACE = -2
    const val KEYCODE_LANG_SWITCH = -3
    const val KEYCODE_TRANSLIT = -4
    const val KEYCODE_SPACE = 32
    const val KEYCODE_ENTER = -5
    const val KEYCODE_SYMBOLS = -6
    const val KEYCODE_SYMBOLS2 = -7  // #+= secondary symbols page

    fun getLayout(language: Language, shifted: Boolean): List<Row> {
        return when (language) {
            Language.UK    -> if (shifted) ukShifted else ukNormal
            Language.EN    -> if (shifted) enShifted else enNormal
            Language.QUC   -> if (shifted) qucShifted else qucNormal
            // All Latin European languages share QWERTY + diacritics strip
            else           -> latinQwertyLayout(language.label, shifted)
        }
    }

    // ── Factory: Latin QWERTY (DE, FR, ES, PT, NO, ES_GT) ──

    private fun latinQwertyLayout(label: String, shifted: Boolean): List<Row> {
        fun k(lower: String) = Key(if (shifted) lower.uppercase() else lower)
        return listOf(
            Row("qwertyuiop".map { k(it.toString()) }),
            Row("asdfghjkl".map { k(it.toString()) }),
            Row(
                listOf(Key("\u2191", KEYCODE_SHIFT, 1.5f, true, "shift")) +
                "zxcvbnm".map { k(it.toString()) } +
                listOf(Key("\u232B", KEYCODE_BACKSPACE, 1.5f, true, "backspace"))
            ),
            Row(listOf(
                Key("123", KEYCODE_SYMBOLS, 1.2f, true),
                Key(label, KEYCODE_LANG_SWITCH, 1f, true),
                Key(" ", KEYCODE_SPACE, 4.5f, true),
                Key(".", code = '.'.code),
                Key("Tr", KEYCODE_TRANSLIT, 1f, true),
                Key("\u21B5", KEYCODE_ENTER, 1.3f, true, "enter"),
            )),
        )
    }

    // ── Ukrainian ЙЦУКЕН ──

    private val ukNormal = listOf(
        Row(listOf(
            Key("й"), Key("ц"), Key("у"), Key("к"), Key("е"),
            Key("н"), Key("г"), Key("ш"), Key("щ"), Key("з"), Key("х"),
        )),
        Row(listOf(
            Key("ф"), Key("і"), Key("в"), Key("а"), Key("п"),
            Key("р"), Key("о"), Key("л"), Key("д"), Key("ж"), Key("є"),
        )),
        Row(listOf(
            Key("\u2191", KEYCODE_SHIFT, 1.3f, true, "shift"),
            Key("я"), Key("ч"), Key("с"), Key("м"), Key("и"),
            Key("т"), Key("ь"), Key("б"), Key("ю"),
            Key("\u232B", KEYCODE_BACKSPACE, 1.3f, true, "backspace"),
        )),
        Row(listOf(
            Key("123", KEYCODE_SYMBOLS, 1.2f, true),
            Key("УК", KEYCODE_LANG_SWITCH, 1f, true),
            Key("ї"),
            Key("ґ"),
            Key(" ", KEYCODE_SPACE, 3.5f, true),
            Key(".", code = '.'.code),
            Key("Tr", KEYCODE_TRANSLIT, 1f, true),
            Key("\u21B5", KEYCODE_ENTER, 1.3f, true, "enter"),
        )),
    )

    private val ukShifted = listOf(
        Row(listOf(
            Key("Й"), Key("Ц"), Key("У"), Key("К"), Key("Е"),
            Key("Н"), Key("Г"), Key("Ш"), Key("Щ"), Key("З"), Key("Х"),
        )),
        Row(listOf(
            Key("Ф"), Key("І"), Key("В"), Key("А"), Key("П"),
            Key("Р"), Key("О"), Key("Л"), Key("Д"), Key("Ж"), Key("Є"),
        )),
        Row(listOf(
            Key("\u2191", KEYCODE_SHIFT, 1.3f, true, "shift"),
            Key("Я"), Key("Ч"), Key("С"), Key("М"), Key("И"),
            Key("Т"), Key("Ь"), Key("Б"), Key("Ю"),
            Key("\u232B", KEYCODE_BACKSPACE, 1.3f, true, "backspace"),
        )),
        Row(listOf(
            Key("123", KEYCODE_SYMBOLS, 1.2f, true),
            Key("УК", KEYCODE_LANG_SWITCH, 1f, true),
            Key("Ї"),
            Key("Ґ"),
            Key(" ", KEYCODE_SPACE, 3.5f, true),
            Key(".", code = '.'.code),
            Key("Tr", KEYCODE_TRANSLIT, 1f, true),
            Key("\u21B5", KEYCODE_ENTER, 1.3f, true, "enter"),
        )),
    )

    // ── English QWERTY ──

    private val enNormal = listOf(
        Row(listOf(
            Key("q"), Key("w"), Key("e"), Key("r"), Key("t"),
            Key("y"), Key("u"), Key("i"), Key("o"), Key("p"),
        )),
        Row(listOf(
            Key("a"), Key("s"), Key("d"), Key("f"), Key("g"),
            Key("h"), Key("j"), Key("k"), Key("l"),
        )),
        Row(listOf(
            Key("\u2191", KEYCODE_SHIFT, 1.5f, true, "shift"),
            Key("z"), Key("x"), Key("c"), Key("v"),
            Key("b"), Key("n"), Key("m"),
            Key("\u232B", KEYCODE_BACKSPACE, 1.5f, true, "backspace"),
        )),
        Row(listOf(
            Key("123", KEYCODE_SYMBOLS, 1.2f, true),
            Key("EN", KEYCODE_LANG_SWITCH, 1f, true),
            Key(" ", KEYCODE_SPACE, 4.5f, true),
            Key(".", code = '.'.code),
            Key("Tr", KEYCODE_TRANSLIT, 1f, true),
            Key("\u21B5", KEYCODE_ENTER, 1.3f, true, "enter"),
        )),
    )

    private val enShifted = listOf(
        Row(listOf(
            Key("Q"), Key("W"), Key("E"), Key("R"), Key("T"),
            Key("Y"), Key("U"), Key("I"), Key("O"), Key("P"),
        )),
        Row(listOf(
            Key("A"), Key("S"), Key("D"), Key("F"), Key("G"),
            Key("H"), Key("J"), Key("K"), Key("L"),
        )),
        Row(listOf(
            Key("\u2191", KEYCODE_SHIFT, 1.5f, true, "shift"),
            Key("Z"), Key("X"), Key("C"), Key("V"),
            Key("B"), Key("N"), Key("M"),
            Key("\u232B", KEYCODE_BACKSPACE, 1.5f, true, "backspace"),
        )),
        Row(listOf(
            Key("123", KEYCODE_SYMBOLS, 1.2f, true),
            Key("EN", KEYCODE_LANG_SWITCH, 1f, true),
            Key(" ", KEYCODE_SPACE, 4.5f, true),
            Key(".", code = '.'.code),
            Key("Tr", KEYCODE_TRANSLIT, 1f, true),
            Key("\u21B5", KEYCODE_ENTER, 1.3f, true, "enter"),
        )),
    )

    // ── K'iche' (QUC) — ALMG standard orthography ──
    // Letters: a b' ch ch' e i j k k' m n o p q q' r s t t' tz tz' u w x xh y
    // Digraphs ch/tz/xh as single keys; apostrophe for glottalization

    private val qucNormal = listOf(
        Row(listOf(
            Key("q"), Key("w"), Key("e"), Key("r"), Key("t"),
            Key("y"), Key("u"), Key("i"), Key("o"), Key("p"),
        )),
        Row(listOf(
            Key("a"), Key("s"), Key("j"), Key("f"), Key("g"),
            Key("h"), Key("'"), Key("k"), Key("l"),
        )),
        Row(listOf(
            Key("\u2191", KEYCODE_SHIFT, 1.3f, true, "shift"),
            Key("ch", widthMultiplier = 1.4f),
            Key("tz", widthMultiplier = 1.4f),
            Key("xh", widthMultiplier = 1.4f),
            Key("z"), Key("x"), Key("b"), Key("n"), Key("m"),
            Key("\u232B", KEYCODE_BACKSPACE, 1.3f, true, "backspace"),
        )),
        Row(listOf(
            Key("123", KEYCODE_SYMBOLS, 1.2f, true),
            Key("Q'", KEYCODE_LANG_SWITCH, 1f, true),
            Key(" ", KEYCODE_SPACE, 4f, true),
            Key(".", code = '.'.code),
            Key("\u21B5", KEYCODE_ENTER, 1.3f, true, "enter"),
        )),
    )

    private val qucShifted = listOf(
        Row(listOf(
            Key("Q"), Key("W"), Key("E"), Key("R"), Key("T"),
            Key("Y"), Key("U"), Key("I"), Key("O"), Key("P"),
        )),
        Row(listOf(
            Key("A"), Key("S"), Key("J"), Key("F"), Key("G"),
            Key("H"), Key("'"), Key("K"), Key("L"),
        )),
        Row(listOf(
            Key("\u2191", KEYCODE_SHIFT, 1.3f, true, "shift"),
            Key("Ch", widthMultiplier = 1.4f),
            Key("Tz", widthMultiplier = 1.4f),
            Key("Xh", widthMultiplier = 1.4f),
            Key("Z"), Key("X"), Key("B"), Key("N"), Key("M"),
            Key("\u232B", KEYCODE_BACKSPACE, 1.3f, true, "backspace"),
        )),
        Row(listOf(
            Key("123", KEYCODE_SYMBOLS, 1.2f, true),
            Key("Q'", KEYCODE_LANG_SWITCH, 1f, true),
            Key(" ", KEYCODE_SPACE, 4f, true),
            Key(".", code = '.'.code),
            Key("\u21B5", KEYCODE_ENTER, 1.3f, true, "enter"),
        )),
    )

    // ── Symbols / Numbers ──

    fun getSymbolsLayout(): List<Row> = listOf(
        Row(listOf(
            Key("1"), Key("2"), Key("3"), Key("4"), Key("5"),
            Key("6"), Key("7"), Key("8"), Key("9"), Key("0"),
        )),
        Row(listOf(
            Key("@"), Key("#"), Key("\u20B4"), Key("&"), Key("-"),
            Key("("), Key(")"), Key("="), Key("%"),
        )),
        Row(listOf(
            Key("#+=", KEYCODE_SYMBOLS2, 1.5f, true),
            Key("!"), Key("\""), Key("'"), Key(":"),
            Key(";"), Key("/"), Key("?"),
            Key("\u232B", KEYCODE_BACKSPACE, 1.5f, true, "backspace"),
        )),
        Row(listOf(
            Key("ABC", KEYCODE_SYMBOLS, 1.2f, true),
            Key("EN", KEYCODE_LANG_SWITCH, 1f, true),
            Key(","),
            Key(" ", KEYCODE_SPACE, 4f, true),
            Key("."),
            Key("\u21B5", KEYCODE_ENTER, 1.3f, true, "enter"),
        )),
    )

    fun getSymbolsLayout2(): List<Row> = listOf(
        Row(listOf(
            Key("~"), Key("`"), Key("!"), Key("@"), Key("#"),
            Key("\$"), Key("^"), Key("*"), Key("["), Key("]"),
        )),
        Row(listOf(
            Key("{"), Key("}"), Key("|"), Key("\\"), Key("<"),
            Key(">"), Key("_"), Key("="), Key("+"),
        )),
        Row(listOf(
            Key("123", KEYCODE_SYMBOLS, 1.5f, true),
            Key("\""), Key("'"), Key(";"), Key(":"),
            Key("/"), Key("?"),
            Key("\u232B", KEYCODE_BACKSPACE, 1.5f, true, "backspace"),
        )),
        Row(listOf(
            Key("ABC", KEYCODE_SYMBOLS, 1.2f, true),
            Key("EN", KEYCODE_LANG_SWITCH, 1f, true),
            Key(","),
            Key(" ", KEYCODE_SPACE, 4f, true),
            Key("."),
            Key("\u21B5", KEYCODE_ENTER, 1.3f, true, "enter"),
        )),
    )
}
