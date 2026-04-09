# Clavi — Project Specification for Claude Code

## What is Clavi

Clavi (`clavi.dev`) is a cross-platform automatic keyboard layout switcher and language bridge tool.
The name comes from Latin `clavis` = key/keyboard.

**Core problem:** People who write in multiple languages (e.g. Ukrainian + English) constantly
switch keyboard layouts manually — or worse, type entire words in the wrong layout and have to
retype. On top of that, Ukrainian diaspora abroad often lacks a Ukrainian keyboard layout entirely
and resorts to phonetic transliteration like "pryvit" instead of "привіт".

**Clavi solves this automatically, silently, cross-platform.**

---

## Hard Rules — Never Violate

1. **Russian language is permanently banned.** No Russian keyboard map, no Russian dictionary,
   no Russian in any language pack. This is hardcoded in core, not a config option. Any
   `pack_loader` must reject locale `"ru"` unconditionally.
2. **No telemetry, no cloud, no accounts.** Everything runs locally.
3. **Privacy first.** Keystrokes never leave the machine. No logging of typed content.

---

## License

**GPLv3** — strong copyleft ensures forks stay open. Language packs (`packs/`) are licensed
separately under **CC BY-SA 4.0** so communities can contribute dictionaries without touching
the core codebase license.

---

## Tech Stack

| Layer              | Technology                                    | Reason                                                       |
|--------------------|-----------------------------------------------|--------------------------------------------------------------|
| Core daemon        | **C++20**                                     | Developer is C++ expert; llama.cpp is native C++; best perf  |
| Build system       | **CMake 3.25+**                               | Cross-platform, standard                                     |
| Keyboard hook      | **libuiohook**                                | Cross-platform (Linux X11/Wayland, macOS, Windows)           |
| LLM inference      | **llama.cpp**                                 | Native C++, no FFI, lazy-loaded sidecar                      |
| Config format      | **TOML** (via `toml++`)                       | Single format everywhere                                     |
| Tray / minimal UI  | **Native per-platform** or **Dear ImGui**     | Keep it lightweight                                          |
| Toast / overlay    | **Platform-native**                           | Minimal dependencies                                         |
| Browser extension  | **TypeScript**                                | Unavoidable for browser                                      |
| Android IME        | **Kotlin + Canvas**                           | Native IME, no NDK for MVP                                   |
| iOS Keyboard       | **Swift + UIKit**                             | Custom Keyboard Extension                                    |
| Hashing            | **xxHash** (xxh3_64)                          | Fast, well-tested                                            |

---

## Repository Structure

```
clavi/
├── CLAUDE.md                  ← this file
├── LICENSE                    ← GPLv3
├── CMakeLists.txt
├── CMakePresets.json
├── android/                   ← Android IME app (Kotlin)
│   ├── app/src/main/
│   │   ├── java/com/clavi/keyboard/
│   │   │   ├── ClaviIME.kt          # InputMethodService
│   │   │   ├── ClaviKeyboardView.kt  # Custom Canvas keyboard
│   │   │   ├── TranslitEngine.kt     # KMU 2010 transliteration
│   │   │   ├── KeyboardLayout.kt     # Layout definitions (UK/EN/symbols)
│   │   │   ├── ClipboardHistory.kt   # Clipboard strip (v0.2)
│   │   │   └── SettingsActivity.kt   # Onboarding
│   │   └── res/
├── ios/                       ← iOS Custom Keyboard Extension (Swift) — v0.5+
├── core/                      # libclavi-core — platform-agnostic logic
├── daemon/                    # clavid — background process
├── tests/
├── fuzz/
├── packs/                     # Language packs (CC BY-SA 4.0)
├── tools/                     # Python build tools
├── data/                      # Raw word lists, keyboard maps
├── deploy/                    # systemd, launchd, Windows service files
└── extern/                    # Git submodules
```

---

## Detection Algorithm (3 Layers)

### Layer 1 — Deterministic (~80% of cases, <1ms)

1. Remap typed word through each layout map
2. Dictionary lookup: if remapped form exists and typed form doesn't → switch

**Ambiguous words:** if typed exists in BOTH dicts → `NoAction`. Never guess.
**Short words:** never act on ≤2 chars.

### Layer 2 — Statistical (~15% of cases, 1–3ms)

Character n-gram model (3–5 grams). Ukrainian has highly distinctive sequences:
`ий`, `ть`, `нн`, `щ`, `ї`, `є`, `ґ`. Confidence threshold: 0.75.

### Layer 3 — LLM context (~5% of cases, 20–80ms)

`llama.cpp` with Qwen2-0.5B (q4_K_M, ~400MB). Lazy-loaded on first use.
Timeout: 150ms. Runs in separate thread via `std::future`.

---

## Binary File Formats

### `dictionary.bin`
Magic `CLAV`, entry count, flat open-addressing hash table of xxh3_64 hashes. Load factor ≤ 0.7.

### `keyboard_map.bin`
Magic `KMAP`, entry count, pairs of `[4-byte source codepoint][4-byte target codepoint]`. Sorted for binary search.

### `ngram.bin`
Magic `NGRM`, ngram count, ngram size, then `[N bytes UTF-8 ngram][4-byte float log-probability]`.

---

## Thread Model (Desktop)

```
Hook Thread (libuiohook) → lock-free ring buffer → Main Thread (Layer 1+2) → LLM Thread (Layer 3)
```

- Hook thread: pushes `KeyEvent` into SPSC ring buffer (256 capacity).
- Main thread: detection pipeline + IPC socket server.
- LLM thread: lazy-started, communicates via `std::promise`/`std::future`.

---

## UX Principles — Critical, Never Compromise

1. **Always show before acting.** Toast: `"Switched to Укр → [undo]"` for 3 seconds.
2. **Undo stack is mandatory from day one.** Last 10 actions, one hotkey to revert.
3. **Never switch on short words (≤2 chars).** Too many false positives.
4. **Exclusion list.** Per-word, per-app opt-out via `exclusions.toml`.
5. **Fix, don't rewrite.** Text improvement must preserve the user's voice and style.

---

## Performance Targets

| Metric          | Target  | Notes                              |
|-----------------|---------|------------------------------------|
| Layer 1 latency | < 1ms   | Dictionary lookup + remap          |
| Layer 2 latency | 1–3ms   | N-gram scoring                     |
| Layer 3 latency | 20–80ms | LLM inference, 150ms hard timeout  |
| Daemon startup  | < 500ms | Without LLM                        |
| RAM (no LLM)    | < 30MB  | Core + 2 dicts + ngram             |
| RAM (with LLM)  | < 500MB | Qwen2-0.5B q4_K_M ≈ 400MB          |
| CPU idle        | < 0.5%  |                                    |
| Android APK     | < 5MB   | No NDK for MVP                     |

---

## Language Pack Format

```toml
# packs/{locale}/pack.toml
[pack]
locale = "uk"
name = "Ukrainian"
version = "1.0.0"

[features]
switch = true
translit = true
bridge = true

[files]
keyboard_map = "keyboard_map.bin"
dictionary = "dictionary.bin"
ngram = "ngram.bin"
translit = "translit.toml"
```

`pack_loader.cpp` must hardcode: `static constexpr std::array<std::string_view, 1> BLOCKED_LOCALES = {"ru"};`

---

## Feature Roadmap

### Desktop

| Version | Features |
|---------|----------|
| v1.0 ✅ | Core switcher: Layer 1+2, libuiohook, tray, hotkeys, undo, UK+EN pack, CI |
| v1.1 ✅ | Wayland detection, fuzzing, clang-format CI |
| v1.5 ✅ | Translit input mode (KMU 2010), Ctrl+T hotkey, bridge mode |
| v2.0    | Layer 3 LLM (Qwen2-0.5B), more language packs (DE, FR, ES…) |
| v2.5    | Browser extension (Chrome/Firefox via localhost socket) |

### Android (`android/`)

| Version | Features |
|---------|----------|
| v0.1 ✅ | UK ЙЦУКЕН + EN QWERTY keyboard, translit mode (KMU 2010), language switch, symbols |
| v0.2    | **Clipboard history strip** — scrollable bar above keyboard, tap-to-paste, undo strip |
| v0.3    | **Smart diacritics** — context strip for PT/DE/NO/FR (ã ü ø é…), no long-press needed |
| v0.4    | **Text fix** — lightweight local spell-check, "fix don't rewrite" principle |
| v0.5    | **More language packs** — ES, PT, DE, NO + local market packs (es_GT, K'iche'…) |
| v1.0    | **LLM text improvement** — Qwen2-0.5B on-device, style-preserving corrections |

### iOS (`ios/`)

| Version | Features |
|---------|----------|
| v0.5    | Custom Keyboard Extension (Swift): same layout/translit as Android |
| v0.6    | Clipboard history, smart diacritics |
| v1.0    | Full feature parity with Android |

---

## Mobile — Architecture Notes

### Android IME (Kotlin, no NDK for MVP)

On mobile, Clavi **is** the keyboard — there is no "wrong layout" detection because
the user explicitly chose the keyboard. The main value props shift to:

1. **Translit / bridge mode** — type Latin phonetically → get Ukrainian Cyrillic. Killer feature
   for diaspora who lack a Ukrainian hardware keyboard.
2. **Smart diacritics** — languages like Portuguese, German, Norwegian, French require
   letters with diacritics (ã, ü, ø, é). Standard keyboards require long-press which
   interrupts typing flow. Clavi shows a context strip above the keyboard with the most
   likely diacritic variant based on the current word.
3. **Clipboard history** — both Android and iOS hide clipboard history. Clavi exposes it
   as a scrollable strip above the keyboard. Solves the "shake to undo" problem on iOS
   and the hidden clipboard on Android.
4. **Text fix** — lightweight local spell-check that fixes obvious errors without
   rewriting the user's voice or style. Think autocorrect done right, not Grammarly.

**What does NOT apply on mobile:**
- Auto-detection of wrong layout (not applicable — user chose the keyboard)
- System tray icon
- Ctrl+Z undo (replaced by clipboard history + undo strip)

### Key design principle: Speed over features

Grammarly is heavy (40MB+, cloud, subscription). Clavi must be:
- APK < 5MB
- Cold start < 300ms
- No internet required
- No accounts

### Smart Diacritics (v0.3)

Languages with heavy diacritics use:
- Portuguese: ã, â, á, à, é, ê, í, ó, ô, õ, ú, ç
- German: ä, ö, ü, ß
- Norwegian: æ, ø, å
- French: é, è, ê, ë, à, â, ç, î, ï, ô, ù, û
- Guatemalan Spanish (es_GT): same as ES but local vocabulary

Implementation: When user types a base letter in a configured language, the suggestions
strip above the keyboard shows diacritic variants ordered by word-frequency in the
dictionary. Tap once — no long-press needed.

### Local Market Packs

Pack system (same format as desktop `packs/{locale}/pack.toml`) extended to:
- `es` — Spanish
- `es_GT` — Guatemalan Spanish (local toponyms, vocabulary)
- `quc` — K'iche' Maya (20M speakers, no good keyboard exists)
- `pt_BR` — Brazilian Portuguese
- `de` — German
- `no` — Norwegian

Each pack ships: dictionary, keyboard layout, translit rules (if applicable),
diacritics frequency table (new field for mobile).

---

## CI/CD (GitHub Actions)

### `ci.yml` — build + test on every push/PR

Matrix: `ubuntu-24.04`, `macos-14`, `windows-2022` × `Debug`/`Release`.
Steps: checkout with submodules → install deps → cmake preset → build → ctest → clang-tidy/format.

### `release.yml` — on tag `v*`

CPack artifacts: `.deb`, `.rpm`, `.pkg`, `.msi`. Upload to GitHub Releases.

### `fuzz.yml` — weekly cron

Linux, `-fsanitize=fuzzer,address,undefined`, 10 minutes per target.

---

## Code Style

- C++20, no exceptions in hot path (`std::expected` or error codes)
- `[[nodiscard]]` on all functions returning results
- No RTTI (`-fno-rtti`)
- All strings as `std::string_view` in hot path
- Clang-format: Google style, 100 col limit
- Kotlin: official style, `ktlint`

---

## Notes for Claude Code

- `pack_loader.cpp` must hardcode rejection of `"ru"` locale — never make it configurable.
- All config is TOML — no YAML anywhere.
- Ukrainian keyboard map reference: https://kbdlayout.info/kbdur1
- For ambiguous words (in both dicts) → always `NoAction`.
- Toast content: `"Clavi: switched to {locale_name} → [Ctrl+Z to undo]"`.
- Android clipboard API: use `ClipboardManager` + `OnPrimaryClipChangedListener`.
  On Android 10+ clipboard reads are restricted outside of focus — the IME has special
  access via `InputMethodService` context, use it.
- Text fix principle: **fix, don't rewrite**. Correct obvious typos and spelling errors.
  Never change word choice, sentence structure, tone, or style. If unsure → don't fix.
- Smart diacritics: sort variants by frequency in the language pack dictionary, not
  alphabetically. Most common variant should be first (leftmost) in the strip.
