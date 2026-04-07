# Clavi

> Cross-platform automatic keyboard layout switcher and language bridge tool.
> *clavis* (Latin) = key / keyboard.

**Clavi solves the problem of typing in the wrong keyboard layout — automatically, silently, cross-platform.**

Type `ghbdsn` when your layout is set to English → Clavi detects it, switches to Ukrainian, and retypes it as `привіт`. Instantly.

## Features (v1.0)

- **Automatic detection** — deterministic dictionary lookup (Layer 1) + n-gram statistics (Layer 2)
- **Non-destructive** — always shows a toast before acting; Ctrl+Z reverts any change
- **Zero cloud** — everything runs locally, no telemetry, no accounts
- **Ukrainian ↔ English** (primary language pair, more packs coming)
- **Exclusion lists** — per-word and per-app opt-out
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
│   ├── include/clavi/ # Public headers
│   └── src/           # Implementation
├── daemon/            # clavid — background process (v1.0 in progress)
├── tests/unit/        # Catch2 unit tests
├── packs/             # Language packs (CC BY-SA 4.0)
│   ├── uk/            # Ukrainian (primary)
│   └── en/            # English
├── tools/             # Python build tools for binary pack data
├── data/              # Raw source data (word lists, keyboard maps)
├── deploy/            # Service files (systemd, launchd, Windows)
└── extern/            # Git submodules (Catch2, toml++, xxHash, libuiohook)
```

## Detection Algorithm

### Layer 1 — Deterministic (~80% of cases, <1ms)
1. Remap typed word through each layout map
2. Dictionary lookup: if remapped form exists and typed form doesn't → switch

### Layer 2 — Statistical (~15% of cases, 1–3ms)
Character n-gram model (3–5 grams). Ukrainian has highly distinctive sequences: `ий`, `ть`, `нн`, `щ`, `ї`, `є`, `ґ`. Confidence threshold: 0.75.

### Layer 3 — LLM context (~5% of cases, 20–80ms) — v2.0
llama.cpp with Qwen2-0.5B (q4_K_M). Lazy-loaded on first use.

## Configuration

Main config: `~/.config/clavi/config.toml`

```toml
[general]
enabled = true
active_pair = ["uk", "en"]

[hotkeys]
toggle = "Ctrl+Shift+Space"
undo = "Ctrl+Z"

[detection]
layer2_threshold = 0.75
layer3_enabled = false
```

Exclusions: `~/.config/clavi/exclusions.toml`

```toml
[words]
skip = ["git", "npm", "sudo"]

[apps]
skip = ["terminal", "code"]
```

## License

Core: **GPLv3**
Language packs (`packs/`): **CC BY-SA 4.0**
