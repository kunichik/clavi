# GitHub Copilot Instructions — Clavi

## Project Overview
Clavi is a cross-platform keyboard layout switcher and language bridge (Latin `clavis` = key).
It auto-detects wrong layouts on desktop and provides a native mobile keyboard (Android + iOS)
with Ukrainian transliteration, smart diacritics, clipboard history, and local text correction.

## Critical Rules

### Russian Language — HARD BAN
Never suggest, generate, or accept any code that:
- Adds `"ru"` as a supported locale
- Includes Russian keyboard layout
- Adds Russian words to any dictionary
- Contains Russian-language UI strings

This is hardcoded policy, not configuration.

### No Network Calls in Keyboard Code
`ClaviIME`, `ClaviKeyboardView`, `TextFixEngine`, `DiacriticsEngine`, `TranslitEngine`,
`ClipboardHistory` — none of these may make HTTP requests or any I/O beyond local storage.

## Android (Primary Platform)

**Package:** `com.clavi.keyboard`  
**Min SDK:** 26 | **Target:** 35 | **Language:** Kotlin | **Build:** Gradle Kotlin DSL

### Key classes and their jobs:
```
ClaviIME            InputMethodService — routes keys, owns state, calls engines
ClaviKeyboardView   Canvas view — draws keyboard rows + one of three strips
KeyboardLayout      Data: UK ЙЦУКЕН, EN QWERTY, symbols rows as List<Row>
TranslitEngine      KMU 2010 translit: shch→щ, zh→ж, ch→ч, sh→ш, etc.
DiacriticsEngine    suggest(char, locale): List<String> — frequency-ordered variants
ClipboardHistory    OnPrimaryClipChangedListener — max 20 items, deduplication
TextFixEngine       analyze(textBefore): Fix? — typo correction, "fix don't rewrite"
SettingsActivity    SharedPreferences: diacritics_locale, default_language
```

### Strip rendering priority in `ClaviKeyboardView.onDraw()`:
```kotlin
when {
    fixSuggestion != null  -> drawFixStrip(canvas, density)
    diacriticItems.isNotEmpty() -> drawDiacriticsStrip(canvas, density)
    clipItems.isNotEmpty() -> drawStrip(canvas, density)
}
```
Do not change this priority order.

### SharedPreferences keys (from SettingsActivity companion):
- `PREFS_NAME = "clavi_prefs"`
- `PREF_DIACRITICS_LOCALE = "diacritics_locale"` — e.g. `"pt"`, `"de"`, null = off
- `PREF_DEFAULT_LANGUAGE = "default_language"` — `"UK"` or `"EN"`

### Common pattern — committing text:
```kotlin
val ic = currentInputConnection ?: return
ic.commitText(text, 1)
```

### Kotlin gotcha with Paint.apply{}:
```kotlin
val sz = 13f * density
// Inside apply{}, use this.textSize to avoid shadowing the outer val:
val paint = Paint().apply { this.textSize = sz }
```

## iOS

**Language:** Swift | **Framework:** UIKit | **Extension:** Custom Keyboard  
**Key file:** `KeyboardViewController.swift` (UIInputViewController subclass)

iOS and Android must stay **logically identical** for:
- Translit rules (same KMU 2010 table)
- Diacritics variants (same frequency tables)
- Keyboard layout structure (same key arrangement)

## TextFixEngine Rules (Important for Copilot Suggestions)

Only suggest fixes that are:
- Unambiguous (typo has one obvious correction)
- High-confidence (not dependent on context/meaning)
- Non-destructive (preserves user's word choice and voice)

Appropriate: `recieve→receive`, `teh→the`, double space, `компютер→комп'ютер`  
Not appropriate: style improvements, word substitutions, grammar rephrasing

## Build Commands
```bash
# Android debug APK
cd android && ./gradlew assembleDebug

# Android checks
./gradlew lint
```

## Code Style
- Kotlin: official style guide, ktlint enforced
- Swift: SwiftLint
- No YAML (use TOML for config files)
- No deprecated APIs (`android.inputmethodservice.Keyboard` is deprecated — use Canvas)
