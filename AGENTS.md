# Clavi — Agent Instructions

> For AI coding assistants (OpenAI Codex, Claude Code, Cursor, Windsurf, Cline, Copilot, etc.)
> Read this before making any changes to the codebase.

## What is Clavi

Cross-platform keyboard layout switcher and language bridge. Latin `clavis` = key.

**Problem:** People writing in multiple languages (Ukrainian + English) constantly switch layouts
manually — or type entire sentences in the wrong layout. Ukrainian diaspora abroad lacks a Ukrainian
hardware keyboard and resorts to phonetic typing ("pryvit" instead of "привіт").

**Solution:** Clavi detects wrong layout automatically (desktop) and provides a native mobile
keyboard with translit mode, smart diacritics, clipboard history, and local text-fix.

---

## Hard Rules — Never Violate

| Rule | Detail |
|------|--------|
| **No cloud, no telemetry** | Everything runs locally. No network calls from core logic. No analytics. |
| **No accounts** | Zero sign-up, zero login, zero tracking. |
| **Privacy first** | Keystrokes never leave the device. No content logging. |
| **License** | GPLv3 (core) + CC BY-SA 4.0 (language packs in `packs/`). |

---

## Repository Map

```
clavi/
├── CLAUDE.md              ← full project spec (read this too)
├── AGENTS.md              ← this file
├── android/               ← Android IME (Kotlin, API 26+, no NDK)
│   └── app/src/main/java/com/clavi/keyboard/
│       ├── ClaviIME.kt          # InputMethodService — all key handling
│       ├── ClaviKeyboardView.kt  # Canvas-drawn keyboard + strips
│       ├── KeyboardLayout.kt     # Layout defs: UK/EN/symbols
│       ├── TranslitEngine.kt     # KMU 2010 transliteration
│       ├── DiacriticsEngine.kt   # Smart diacritics variants
│       ├── ClipboardHistory.kt   # Clipboard strip (max 20 items)
│       ├── TextFixEngine.kt      # Local spell-fix (fix, don't rewrite)
│       └── SettingsActivity.kt   # Onboarding + preferences
├── ios/ClaviKeyboard/ClaviKeyboardExtension/
│   ├── KeyboardViewController.swift  # UIInputViewController
│   ├── ClaviKeyboardView.swift       # UIView with UIButton keys
│   ├── TranslitEngine.swift          # Same logic as Kotlin
│   ├── KeyboardLayout.swift          # Same layouts as Kotlin
│   ├── DiacriticsEngine.swift        # Same tables as Kotlin
│   └── ClipboardHistory.swift        # UIPasteboard-based
├── core/                  ← libclavi-core (C++20) — not yet in repo
├── packs/                 ← Language packs (CC BY-SA 4.0)
└── .github/workflows/
    └── android.yml        ← CI: builds debug + release APK
```

---

## Platform Build Commands

### Android
```bash
cd android
./gradlew assembleDebug        # debug APK → app/build/outputs/apk/debug/
./gradlew assembleRelease      # unsigned release APK
./gradlew lint                 # lint check
```
Requires JDK 17. Min SDK 26, target SDK 35.

### iOS
Open `ios/ClaviKeyboard/ClaviKeyboard.xcodeproj` in Xcode 15+.
Build the `ClaviKeyboardExtension` target. Requires macOS + physical device for
full clipboard access (simulator has restrictions).

### Desktop Core (future)
```bash
cmake --preset default
cmake --build build --parallel
ctest --test-dir build
```

---

## Android Architecture

The keyboard is a **custom `InputMethodService`** — it IS the keyboard, not a hook into one.

```
ClaviIME (InputMethodService)
  ├── ClaviKeyboardView (Canvas)
  │     ├── Strip area (top ~48dp): fix → diacritics → clipboard (priority order)
  │     └── Key rows (remaining height)
  ├── TranslitEngine — digraph buffer, KMU 2010 rules
  ├── ClipboardHistory — ClipboardManager listener, max 20 items
  ├── DiacriticsEngine — static tables, frequency-ordered
  └── TextFixEngine — analyze(textBefore): Fix? — runs after space key
```

**Strip priority** (only one strip shown at a time):
1. Fix suggestion (green) — TextFixEngine found a fixable typo
2. Diacritics (teal) — user just typed a base letter with variants
3. Clipboard history (dark) — default state

**Key state flow:**
- `handleCharacter()` → commit text → check diacritics → if space, run TextFixEngine
- `handleBackspace()` → clear diacritics + fix suggestion, delete char or pop translit buffer
- Translit buffer: Latin chars accumulate in `StringBuilder`, flushed when a match is found

---

## iOS Architecture

`UIInputViewController` subclass. Uses `UIButton`-based keys (not Canvas). Clipboard via
`UIPasteboard.general` — read access requires `RequestsOpenAccess = true` in Info.plist.

Mirror principle: **iOS and Android implementations must stay in sync**. Same layout tables,
same translit rules, same diacritics tables, same strip UX.

---

## Key Design Principles

### 1. Fix, Don't Rewrite
`TextFixEngine` corrects obvious errors only:
- Double space, space before punctuation, missing space after sentence-end
- High-confidence typos (`teh→the`, `recieve→receive`, `компютер→комп'ютер`)
- Repeated characters (`hellllo→hello`)

**Never:** change word choice, sentence structure, style, or tone. If unsure → return null.

### 2. Never Guess on Short Words
Desktop detection: never act on ≤ 2 characters. Too many false positives.

### 3. Ambiguous = No Action
If a word exists in both the Ukrainian and English dictionary → `NoAction`. Never guess.

### 4. Speed Over Features
- APK < 5 MB (no NDK, no heavy libs)
- Cold start < 300 ms
- TextFixEngine < 2 ms
- No network at runtime

### 5. Ukrainian First
Ukrainian is the primary non-English language. KMU 2010 standard for transliteration.
Ukrainian keyboard: ЙЦУКЕН layout with `ї`, `є`, `ґ` in bottom row.

---

## Translit Rules (KMU 2010)

Priority: trigraphs → digraphs → single chars

| Input | Output | Note |
|-------|--------|------|
| `shch` | `щ` | trigraph — must check first |
| `zh` | `ж` | |
| `kh` | `х` | |
| `ts` | `ц` | |
| `ch` | `ч` | |
| `sh` | `ш` | |
| `iu` | `ю` | |
| `ia` | `я` | |
| `ie` | `є` | |
| `yi` | `ї` | |
| `y` | `й` | single |
| `j` | `й` | alias |

Digraph buffer: don't commit `s` immediately — it might be the start of `sh` or `shch`.
`isDigraphStart(c)` returns true for `s`, `z`, `k`, `t`, `c`, `i`, `y`.

---

## What NOT to Do

- Do NOT add network calls to `ClaviIME`, `ClaviKeyboardView`, or any engine class
- Do NOT use `android.inputmethodservice.Keyboard` (deprecated)
- Do NOT use `RecyclerView` in the keyboard view — Canvas only for performance
- Do NOT add Grammarly-style rewrites to TextFixEngine — fix typos only
- Do NOT break the strip priority order (fix > diacritics > clipboard)
- Do NOT add features not on the roadmap without discussion
- Do NOT commit `.env`, credentials, or API keys

---

## Roadmap Context

### Android — current state
| Version | Status | Features |
|---------|--------|----------|
| v0.1 | ✅ done | UK + EN keyboard, translit, language switch, symbols |
| v0.2 | ✅ done | Clipboard history strip |
| v0.3 | ✅ done | Smart diacritics strip |
| v0.4 | ✅ done | TextFix engine |
| v0.5 | 🔄 next | More language packs (ES, PT, DE, NO), settings UX |
| v1.0 | planned | On-device LLM (Qwen2-0.5B), full settings |

### iOS — current state
| Version | Status | Features |
|---------|--------|----------|
| v0.5 | ✅ done | UK + EN keyboard, translit, clipboard, diacritics |
| v0.6 | 🔄 next | Settings, text fix parity with Android |

---

## Code Style

- **Kotlin**: official style guide, `ktlint` — 4-space indent, no semicolons
- **Swift**: SwiftLint — 4-space indent
- **C++**: Google style, clang-format, 100-col limit, C++20, no exceptions in hot path
- **No**: YAML (use TOML), cloud calls, heavy dependencies

---

## Contribution Checklist

Before submitting a PR:
- [ ] `./gradlew assembleDebug` passes (Android)
- [ ] If touching translit: verify KMU 2010 table is unchanged
- [ ] If touching Android strip: verify fix > diacritics > clipboard priority
- [ ] If adding a language pack: CC BY-SA 4.0 license
- [ ] If touching `TextFixEngine`: only add high-confidence, unambiguous fixes
- [ ] No network calls added
