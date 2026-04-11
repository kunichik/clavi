import Foundation
import UIKit

enum Language: String, CaseIterable {
    case uk, en
    case de, fr, es, pt, no, esGT = "es_GT"
    case quc
}

extension Language {
    /// Short label shown on the language-switch key.
    var keyLabel: String {
        switch self {
        case .uk:   return "УК"
        case .en:   return "EN"
        case .de:   return "DE"
        case .fr:   return "FR"
        case .es:   return "ES"
        case .pt:   return "PT"
        case .no:   return "NO"
        case .esGT: return "GT"
        case .quc:  return "Q'"
        }
    }

    /// BCP 47 locale to auto-enable diacritics for when this language is active. Nil = no auto-diacritics.
    var diacriticsLocale: String? {
        switch self {
        case .de:          return "de"
        case .fr:          return "fr"
        case .es, .esGT:   return "es"
        case .pt:          return "pt"
        case .no:          return "no"
        default:           return nil
        }
    }

    /// Display name for Settings.
    var displayName: String {
        switch self {
        case .uk:   return "Ukrainian (УК)"
        case .en:   return "English (EN)"
        case .de:   return "German (DE)"
        case .fr:   return "French (FR)"
        case .es:   return "Spanish (ES)"
        case .pt:   return "Portuguese (PT)"
        case .no:   return "Norwegian (NO)"
        case .esGT: return "Guatemalan Spanish (GT)"
        case .quc:  return "K'iche' Maya (Q')"
        }
    }
}

enum KeyAction {
    case character(String)
    case shift
    case backspace
    case langSwitch
    case translit
    case symbols
    case enter
    case space
}

struct KeyDef {
    let label: String
    let action: KeyAction
    let widthWeight: CGFloat
    let isSpecial: Bool

    init(_ label: String, _ action: KeyAction, weight: CGFloat = 1, special: Bool = false) {
        self.label = label
        self.action = action
        self.widthWeight = weight
        self.isSpecial = special
    }
}

enum KeyboardLayout {

    static func layout(for language: Language, shifted: Bool) -> [[KeyDef]] {
        switch language {
        case .uk:  return shifted ? ukShifted : ukNormal
        case .en:  return shifted ? enShifted : enNormal
        case .quc: return shifted ? qucShifted : qucNormal
        // All Latin European languages share QWERTY + diacritics strip
        default:   return latinQwerty(label: language.keyLabel, shifted: shifted)
        }
    }

    // MARK: - Factory: Latin QWERTY (DE, FR, ES, PT, NO, ES_GT)

    private static func latinQwerty(label: String, shifted: Bool) -> [[KeyDef]] {
        let r1 = "qwertyuiop"
        let r2 = "asdfghjkl"
        let r3 = "zxcvbnm"
        func c(_ s: String) -> String { shifted ? s.uppercased() : s }
        return [
            r1.map { .init(c(String($0)), .character(c(String($0)))) },
            r2.map { .init(c(String($0)), .character(c(String($0)))) },
            [.init("⇧", .shift, weight: 1.5, special: true)] +
            r3.map { .init(c(String($0)), .character(c(String($0)))) } +
            [.init("⌫", .backspace, weight: 1.5, special: true)],
            [.init("123", .symbols, weight: 1.2, special: true),
             .init(label, .langSwitch, special: true),
             .init(" ", .space, weight: 4.5, special: true),
             .init(".", .character(".")),
             .init("Tr", .translit, special: true),
             .init("⏎", .enter, weight: 1.3, special: true)],
        ]
    }

    // MARK: - Ukrainian ЙЦУКЕН

    static let ukNormal: [[KeyDef]] = [
        [.init("й",.character("й")), .init("ц",.character("ц")), .init("у",.character("у")),
         .init("к",.character("к")), .init("е",.character("е")), .init("н",.character("н")),
         .init("г",.character("г")), .init("ш",.character("ш")), .init("щ",.character("щ")),
         .init("з",.character("з")), .init("х",.character("х"))],
        [.init("ф",.character("ф")), .init("і",.character("і")), .init("в",.character("в")),
         .init("а",.character("а")), .init("п",.character("п")), .init("р",.character("р")),
         .init("о",.character("о")), .init("л",.character("л")), .init("д",.character("д")),
         .init("ж",.character("ж")), .init("є",.character("є"))],
        [.init("⇧",.shift,weight:1.3,special:true),
         .init("я",.character("я")), .init("ч",.character("ч")), .init("с",.character("с")),
         .init("м",.character("м")), .init("и",.character("и")), .init("т",.character("т")),
         .init("ь",.character("ь")), .init("б",.character("б")), .init("ю",.character("ю")),
         .init("⌫",.backspace,weight:1.3,special:true)],
        [.init("123",.symbols,weight:1.2,special:true),
         .init("УК",.langSwitch,special:true),
         .init("ї",.character("ї")), .init("ґ",.character("ґ")),
         .init(" ",.space,weight:3.5,special:true),
         .init(".",.character(".")),
         .init("Tr",.translit,special:true),
         .init("⏎",.enter,weight:1.3,special:true)],
    ]

    static let ukShifted: [[KeyDef]] = [
        [.init("Й",.character("Й")), .init("Ц",.character("Ц")), .init("У",.character("У")),
         .init("К",.character("К")), .init("Е",.character("Е")), .init("Н",.character("Н")),
         .init("Г",.character("Г")), .init("Ш",.character("Ш")), .init("Щ",.character("Щ")),
         .init("З",.character("З")), .init("Х",.character("Х"))],
        [.init("Ф",.character("Ф")), .init("І",.character("І")), .init("В",.character("В")),
         .init("А",.character("А")), .init("П",.character("П")), .init("Р",.character("Р")),
         .init("О",.character("О")), .init("Л",.character("Л")), .init("Д",.character("Д")),
         .init("Ж",.character("Ж")), .init("Є",.character("Є"))],
        [.init("⇧",.shift,weight:1.3,special:true),
         .init("Я",.character("Я")), .init("Ч",.character("Ч")), .init("С",.character("С")),
         .init("М",.character("М")), .init("И",.character("И")), .init("Т",.character("Т")),
         .init("Ь",.character("Ь")), .init("Б",.character("Б")), .init("Ю",.character("Ю")),
         .init("⌫",.backspace,weight:1.3,special:true)],
        [.init("123",.symbols,weight:1.2,special:true),
         .init("УК",.langSwitch,special:true),
         .init("Ї",.character("Ї")), .init("Ґ",.character("Ґ")),
         .init(" ",.space,weight:3.5,special:true),
         .init(".",.character(".")),
         .init("Tr",.translit,special:true),
         .init("⏎",.enter,weight:1.3,special:true)],
    ]

    // MARK: - English QWERTY

    static let enNormal: [[KeyDef]] = [
        [.init("q",.character("q")), .init("w",.character("w")), .init("e",.character("e")),
         .init("r",.character("r")), .init("t",.character("t")), .init("y",.character("y")),
         .init("u",.character("u")), .init("i",.character("i")), .init("o",.character("o")),
         .init("p",.character("p"))],
        [.init("a",.character("a")), .init("s",.character("s")), .init("d",.character("d")),
         .init("f",.character("f")), .init("g",.character("g")), .init("h",.character("h")),
         .init("j",.character("j")), .init("k",.character("k")), .init("l",.character("l"))],
        [.init("⇧",.shift,weight:1.5,special:true),
         .init("z",.character("z")), .init("x",.character("x")), .init("c",.character("c")),
         .init("v",.character("v")), .init("b",.character("b")), .init("n",.character("n")),
         .init("m",.character("m")),
         .init("⌫",.backspace,weight:1.5,special:true)],
        [.init("123",.symbols,weight:1.2,special:true),
         .init("EN",.langSwitch,special:true),
         .init(" ",.space,weight:4.5,special:true),
         .init(".",.character(".")),
         .init("Tr",.translit,special:true),
         .init("⏎",.enter,weight:1.3,special:true)],
    ]

    static let enShifted: [[KeyDef]] = [
        [.init("Q",.character("Q")), .init("W",.character("W")), .init("E",.character("E")),
         .init("R",.character("R")), .init("T",.character("T")), .init("Y",.character("Y")),
         .init("U",.character("U")), .init("I",.character("I")), .init("O",.character("O")),
         .init("P",.character("P"))],
        [.init("A",.character("A")), .init("S",.character("S")), .init("D",.character("D")),
         .init("F",.character("F")), .init("G",.character("G")), .init("H",.character("H")),
         .init("J",.character("J")), .init("K",.character("K")), .init("L",.character("L"))],
        [.init("⇧",.shift,weight:1.5,special:true),
         .init("Z",.character("Z")), .init("X",.character("X")), .init("C",.character("C")),
         .init("V",.character("V")), .init("B",.character("B")), .init("N",.character("N")),
         .init("M",.character("M")),
         .init("⌫",.backspace,weight:1.5,special:true)],
        [.init("123",.symbols,weight:1.2,special:true),
         .init("EN",.langSwitch,special:true),
         .init(" ",.space,weight:4.5,special:true),
         .init(".",.character(".")),
         .init("Tr",.translit,special:true),
         .init("⏎",.enter,weight:1.3,special:true)],
    ]

    // MARK: - K'iche' (QUC)

    static let qucNormal: [[KeyDef]] = [
        [.init("q",.character("q")), .init("w",.character("w")), .init("e",.character("e")),
         .init("r",.character("r")), .init("t",.character("t")), .init("y",.character("y")),
         .init("u",.character("u")), .init("i",.character("i")), .init("o",.character("o")),
         .init("p",.character("p"))],
        [.init("a",.character("a")), .init("s",.character("s")), .init("j",.character("j")),
         .init("f",.character("f")), .init("g",.character("g")), .init("h",.character("h")),
         .init("'",.character("'")), .init("k",.character("k")), .init("l",.character("l"))],
        [.init("⇧",.shift,weight:1.3,special:true),
         .init("ch",.character("ch"),weight:1.4),
         .init("tz",.character("tz"),weight:1.4),
         .init("xh",.character("xh"),weight:1.4),
         .init("z",.character("z")), .init("x",.character("x")),
         .init("b",.character("b")), .init("n",.character("n")), .init("m",.character("m")),
         .init("⌫",.backspace,weight:1.3,special:true)],
        [.init("123",.symbols,weight:1.2,special:true),
         .init("Q'",.langSwitch,special:true),
         .init(" ",.space,weight:4.0,special:true),
         .init(".",.character(".")),
         .init("⏎",.enter,weight:1.3,special:true)],
    ]

    static let qucShifted: [[KeyDef]] = [
        [.init("Q",.character("Q")), .init("W",.character("W")), .init("E",.character("E")),
         .init("R",.character("R")), .init("T",.character("T")), .init("Y",.character("Y")),
         .init("U",.character("U")), .init("I",.character("I")), .init("O",.character("O")),
         .init("P",.character("P"))],
        [.init("A",.character("A")), .init("S",.character("S")), .init("J",.character("J")),
         .init("F",.character("F")), .init("G",.character("G")), .init("H",.character("H")),
         .init("'",.character("'")), .init("K",.character("K")), .init("L",.character("L"))],
        [.init("⇧",.shift,weight:1.3,special:true),
         .init("Ch",.character("Ch"),weight:1.4),
         .init("Tz",.character("Tz"),weight:1.4),
         .init("Xh",.character("Xh"),weight:1.4),
         .init("Z",.character("Z")), .init("X",.character("X")),
         .init("B",.character("B")), .init("N",.character("N")), .init("M",.character("M")),
         .init("⌫",.backspace,weight:1.3,special:true)],
        [.init("123",.symbols,weight:1.2,special:true),
         .init("Q'",.langSwitch,special:true),
         .init(" ",.space,weight:4.0,special:true),
         .init(".",.character(".")),
         .init("⏎",.enter,weight:1.3,special:true)],
    ]

    // MARK: - Symbols

    static let symbols: [[KeyDef]] = [
        [.init("1",.character("1")), .init("2",.character("2")), .init("3",.character("3")),
         .init("4",.character("4")), .init("5",.character("5")), .init("6",.character("6")),
         .init("7",.character("7")), .init("8",.character("8")), .init("9",.character("9")),
         .init("0",.character("0"))],
        [.init("@",.character("@")), .init("#",.character("#")), .init("₴",.character("₴")),
         .init("&",.character("&")), .init("-",.character("-")), .init("(",.character("(")),
         .init(")",.character(")")), .init("=",.character("=")), .init("%",.character("%"))],
        [.init("ABC",.symbols,weight:1.5,special:true),
         .init("!",.character("!")), .init("\"",.character("\"")), .init("'",.character("'")),
         .init(":",.character(":")), .init(";",.character(";")), .init("/",.character("/")),
         .init("?",.character("?")),
         .init("⌫",.backspace,weight:1.5,special:true)],
        [.init("ABC",.symbols,weight:1.2,special:true),
         .init(",",.character(",")),
         .init(" ",.space,weight:4.5,special:true),
         .init(".",.character(".")),
         .init("⏎",.enter,weight:1.3,special:true)],
    ]
}
