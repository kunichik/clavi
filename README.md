# Clavi

> Cross-platform automatic keyboard layout switcher and language bridge tool.
> *clavis* (Latin) = key / keyboard.

**Clavi solves the problem of typing in the wrong keyboard layout — automatically, silently, cross-platform.**

Type `ghbdsn` when your layout is set to English → Clavi detects it, switches to Ukrainian, and retypes it as `привіт`. Instantly.

## Features

- **Automatic detection** — deterministic dictionary lookup (Layer 1) + n-gram statistics (Layer 2)
- **Non-destructive** — toast notification before acting; Ctrl+Z reverts any change
- **Translit input mode** — press Ctrl+T, type Latin phonetically, get Ukrainian Cyrillic (KMU 2010 standard)
- **Bridge mode** — always-on translit: every word auto-converts without layout switching (`--mode bridge`)
- **System tray icon** — right-click context menu: Pause / Enable / Quit (Windows)
- **Zero cloud** — everything runs locally, no telemetry, no accounts
- **Ukrainian ↔ English** (primary pair; pack format supports any language)
- **Exclusion lists** — per-word and per-app opt-out via `exclusions.toml`
- **Diagnostic logging** — rotating file log (5 MB × 3), privacy-safe (no keystrokes logged)
- **Cross-platform** — Linux (X11/XWayland), macOS, Windows

## Hard Rules

1. **Russian language is permanently disabled.** No Russian keyboard map, no Russian dictionary, ever.
2. **No telemetry, no cloud, no accounts.**
3. **Keystrokes never leave the machine.**

## Tech Stack

| Layer | Technology |
|-------|-----------|
| Core daemon | C++20 |
| Build | CMake 3.25+ |
| Keyboard hook | libuiohook |
| LLM (v2.0) | llama.cpp (Qwen2-0.5B) |
| Config | TOML (toml++) |
| Tests | Catch2 |
| Hashing | xxHash (xxh3_64) |

## Building

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/yourname/clavi
cd clavi

# Install Python deps for build tools
pip install -r tools/requirements.txt

# Download word lists (from hunspell uk_UA + dwyl/english-words)
python3 tools/download_wordlists.py

# Generate binary pack data
python3 tools/build_keymap.py data/keyboard_maps/en_qwerty.json data/keyboard_maps/uk_qwerty.json packs/uk/keyboard_map.bin
python3 tools/build_keymap.py data/keyboard_maps/uk_qwerty.json data/keyboard_maps/en_qwerty.json packs/en/keyboard_map.bin
python3 tools/build_dict.py data/wordlists/uk_words.txt packs/uk/dictionary.bin
python3 tools/build_dict.py data/wordlists/en_words.txt packs/en/dictionary.bin
python3 tools/build_ngram.py data/wordlists/uk_words.txt packs/uk/ngram.bin --ngram-size 3

# Configure + build
cmake --preset linux-debug    # or macos-debug, windows-debug
cmake --build build/linux-debug

# Run tests
ctest --test-dir build/linux-debug --output-on-failure
```

## Repository Structure

```
clavi/
├── core/              # libclavi-core — platform-agnostic detection engine
│   ├── include/clavi/ # Public headers (detector, layout_map, translit, logger, …)
│   └── src/           # Implementation
├── daemon/            # clavid — background process
│   ├── include/clavi/
│   │   └── platform/  # tray.hpp, toast.hpp, switcher.hpp
│   └── src/
│       ├── platform/  # tray/toast/switcher impls per OS
│       ├── hook.cpp   # libuiohook keyboard hook
│       ├── word_buffer.cpp
│       └── main.cpp
├── tests/
│   ├── unit/          # 97 unit test cases
│   └── integration/   # 10 integration test cases (real packs)
├── fuzz/              # libFuzzer targets (detector, layout_map, ngram_model)
├── packs/             # Language packs (CC BY-SA 4.0)
│   ├── uk/            # Ukrainian (dictionary, keyboard_map, ngram, translit)
│   └── en/            # English
├── tools/             # Python build tools for binary pack data
├── data/              # Raw source data (word lists, keyboard maps)
├── docs/              # config-example.toml and other docs
├── deploy/            # Service files (systemd, launchd, Windows)
└── extern/            # Git submodules (Catch2, toml++, xxHash, libuiohook)
```

## Test Suite

| Suite | Cases | Assertions |
|-------|-------|------------|
| Unit (core + daemon) | 97 | ~357 |
| Integration (real packs) | 10 | 46 |
| **Total** | **107** | **403** |

Run: `ctest --test-dir build/<preset> --output-on-failure`

## Detection Algorithm

### Layer 1 — Deterministic (~80% of cases, <1ms)
1. Remap typed word through each layout map
2. Dictionary lookup: if remapped form exists and typed form doesn't → switch

### Layer 2 — Statistical (~15% of cases, 1–3ms)
Character n-gram model (3–5 grams). Ukrainian has highly distinctive sequences: `ий`, `ть`, `нн`, `щ`, `ї`, `є`, `ґ`. Confidence threshold: 0.75.

### Layer 3 — LLM context (~5% of cases, 20–80ms) — v2.0
llama.cpp with Qwen2-0.5B (q4_K_M). Lazy-loaded on first use.

## Hotkeys

| Hotkey | Action |
|--------|--------|
| `Ctrl+Shift+Space` | Toggle Clavi on/off |
| `Ctrl+Z` | Undo last auto-correction |
| `Ctrl+T` | Toggle translit input mode |

## CLI Usage

```
clavid [options]

  -v, --verbose              Print events to stdout
  --version                  Print version and exit
  --mode <mode>              Override config mode:
                               detection  (auto-detect wrong layout)
                               bridge     (always-on translit, no layout switch)
  --translit-locale <locale> Override translit target locale (e.g. uk)
  --packs <dir>              Override language packs directory
  -h, --help                 Show help
```

**Examples:**

```bash
# Auto-detect mode (default)
clavid

# Bridge mode: type Latin, get Ukrainian, no layout switching
clavid --mode bridge --translit-locale uk

# Point at local packs during development
clavid --packs ./packs -v
```

## Configuration

Main config: `~/.config/clavi/config.toml` (see [`docs/config-example.toml`](docs/config-example.toml) for all options)

```toml
[general]
enabled = true
active_pair = ["uk", "en"]
translit_locale = "uk"   # target for Ctrl+T translit mode
mode = "detection"       # "detection" | "bridge"

[hotkeys]
toggle = "Ctrl+Shift+Space"
undo = "Ctrl+Z"

[detection]
layer2_threshold = 0.75
layer3_enabled = false

[logging]
enabled = false
level = "info"           # debug | info | warn | error
```

Exclusions: `~/.config/clavi/exclusions.toml`

```toml
[words]
skip = ["git", "npm", "sudo"]

[apps]
skip = ["terminal", "code"]
match = "exact"          # "exact" | "substring"
```

## License

Core: **GPLv3**
Language packs (`packs/`): **CC BY-SA 4.0**
