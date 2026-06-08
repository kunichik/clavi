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

Include `LICENSE` (GPLv3 text) and `packs/LICENSE` (CC BY-SA 4.0) in repo root.

---

## Tech Stack

| Layer | Technology | Reason |
|-------|-----------|--------|
| Core daemon | **C++20** | Developer is C++ expert; llama.cpp is native C++; best performance |
| Build system | **CMake 3.25+** | Cross-platform, standard |
| Keyboard hook | **libuiohook** | Cross-platform (Linux X11/Wayland, macOS, Windows), same lib used by KeyboardSwitch |
| LLM inference | **llama.cpp** | Native C++, no FFI, lazy-loaded sidecar |
| Config format | **TOML** (via `toml++`) | Single format everywhere — pack.toml, config.toml, exclusions.toml |
| Tray / minimal UI | **Native per-platform** or **Dear ImGui** | Keep it lightweight |
| Toast / overlay | **Platform-native** (see Toast System section) | Minimal dependencies |
| Browser extension | **TypeScript** | Unavoidable for browser |
| Extension ↔ daemon | **localhost socket** (MVP), then **WASM** (v2) | Simple start |
| Packaging | **CPack** (deb/rpm/msi/pkg) | One CMake command |
| Hashing | **xxHash** (xxh3_64) | Fast, well-tested, used in dictionary.bin and keyboard_map.bin |

---

## Repository Structure

```
clavi/
├── CLAUDE.md                  ← this file
├── LICENSE                    ← GPLv3
├── CMakeLists.txt
├── CMakePresets.json
├── .github/
│   └── workflows/
│       ├── ci.yml             # Build + test on push/PR (Linux, macOS, Windows)
│       ├── release.yml        # CPack artifacts on tag push
│       └── fuzz.yml           # Weekly fuzzing run
├── extern/
│   ├── libuiohook/            # git submodule
│   ├── llama.cpp/             # git submodule
│   ├── tomlplusplus/          # git submodule — TOML parser
│   ├── xxHash/                # git submodule — hash function
│   └── Catch2/                # git submodule — test framework
├── core/                      # libclavi-core — platform-agnostic logic
│   ├── CMakeLists.txt
│   ├── include/clavi/
│   │   ├── detector.hpp       # language detection interface
│   │   ├── layout_map.hpp     # keyboard remapping
│   │   ├── dictionary.hpp     # hash-set word lookup
│   │   ├── translit.hpp       # transliteration engine
│   │   ├── undo_stack.hpp     # undo buffer
│   │   ├── pack_loader.hpp    # language pack plugin loader
│   │   └── config.hpp         # TOML config (uses toml++)
│   └── src/
│       ├── detector.cpp
│       ├── layout_map.cpp
│       ├── dictionary.cpp
│       ├── translit.cpp
│       ├── undo_stack.cpp
│       └── pack_loader.cpp    # BLOCKS "ru" locale hardcoded here
├── daemon/                    # clavid — background process
│   ├── CMakeLists.txt
│   └── src/
│       ├── main.cpp
│       ├── hook.cpp           # libuiohook callbacks
│       ├── socket_server.cpp  # IPC for extension
│       └── platform/
│           ├── switcher_linux.cpp   # xdotool / ibus
│           ├── switcher_mac.cpp     # TIS/AppleScript
│           ├── switcher_win.cpp     # SendInput WinAPI
│           ├── toast_linux.cpp      # libnotify / D-Bus
│           ├── toast_mac.cpp        # NSUserNotification / UNUserNotificationCenter
│           └── toast_win.cpp        # WinToast / Shell_NotifyIcon
├── tests/                     # All tests live here
│   ├── CMakeLists.txt
│   ├── unit/
│   │   ├── test_layout_map.cpp
│   │   ├── test_dictionary.cpp
│   │   ├── test_detector.cpp
│   │   ├── test_translit.cpp
│   │   ├── test_undo_stack.cpp
│   │   ├── test_pack_loader.cpp   # must verify "ru" rejection
│   │   └── test_config.cpp
│   ├── integration/
│   │   ├── test_detector_e2e.cpp  # full Layer 1+2 pipeline
│   │   └── test_daemon_ipc.cpp    # socket server round-trip
│   └── fuzz/
│       ├── fuzz_detector.cpp      # random keycode sequences
│       └── fuzz_layout_map.cpp    # random char → remap
├── packs/                     # Language packs
│   ├── LICENSE                ← CC BY-SA 4.0
│   ├── uk/                    # Ukrainian (primary)
│   │   ├── pack.toml
│   │   ├── keyboard_map.bin
│   │   ├── dictionary.bin     # 200k words
│   │   ├── ngram.bin          # character n-gram model
│   │   └── translit.toml      # KMU 2010 standard
│   └── en/                    # English
│       ├── pack.toml
│       ├── keyboard_map.bin
│       └── dictionary.bin     # 300k words
├── deploy/                    # Daemon service files
│   ├── linux/
│   │   └── clavi.service      # systemd user unit
│   ├── macos/
│   │   └── dev.clavi.daemon.plist  # launchd agent
│   └── windows/
│       └── clavi-service.xml  # NSSM / Windows Service wrapper config
├── extension/                 # Browser extension (Chrome/Firefox)
│   ├── manifest.json
│   ├── src/
│   │   ├── content.ts         # textarea interceptor
│   │   ├── background.ts      # daemon socket client
│   │   └── popup.ts
│   └── wasm/                  # v2: compiled core
├── tools/                     # Dev utilities
│   ├── build_dict.py          # compile word lists → binary hash-set
│   ├── build_ngram.py         # compile n-gram model
│   └── build_keymap.py        # compile JSON keyboard map → keyboard_map.bin
└── data/
    ├── wordlists/
    │   ├── uk_words.txt       # raw Ukrainian word list
    │   └── en_words.txt       # raw English word list
    └── keyboard_maps/
        ├── uk_qwerty.json     # Ukrainian QWERTY layout
        └── en_qwerty.json     # English QWERTY layout
```

---

## Detection Algorithm (3 Layers)

### Layer 1 — Deterministic (handles ~80% of cases, <1ms)

**Step 1: Layout remap**
When user types a word, build two parallel buffers:
- `typed`: what was actually typed (e.g. `ghbdtn`)
- `remapped_uk`: what it would be if the layout were Ukrainian (e.g. `привіт`)
- `remapped_en`: what it would be if the layout were English

**Step 2: Dictionary lookup**
Use `std::unordered_set<std::string_view>` loaded from binary hash-set.
- If `typed` is in English dict and NOT in Ukrainian dict → correct layout, no action
- If `typed` is in Ukrainian dict and NOT in English dict → correct layout, no action
- If `remapped_uk` is in Ukrainian dict and `typed` is NOT → wrong layout! Switch + retype
- If `remapped_en` is in English dict and `typed` is NOT → wrong layout! Switch + retype

**Ambiguous words (cross-language homographs):**
- If `typed` exists in BOTH dictionaries → `Action::NoAction` (don't guess)
- If `remapped_uk` exists in Ukrainian dict AND `remapped_en` exists in English dict → escalate
  to Layer 2 for statistical tiebreaking
- Single-char words handled via exclusion: never act on `≤2 chars` (see UX rule 3)
- Common ambiguous tokens (e.g. `"a"`, `"i"`, `"I"`) are hardcoded in a skip-list
  inside `detector.cpp`

### Layer 2 — Statistical (handles ~15% of cases, 1-3ms)

fastText-style character n-gram model (3-5 grams).
Ukrainian has highly distinctive sequences: `ий`, `ть`, `нн`, `щ`, `ї`, `є`, `ґ`.
Confidence threshold: 0.75. If below → escalate to Layer 3.

### Layer 3 — LLM context (handles ~5% of cases, 20-80ms)

`llama.cpp` with Qwen2-0.5B (q4_K_M, ~400MB).
**Lazy loaded** — model loads only on first Layer 3 call, then stays in memory.
Input: last 8-10 words of context + new word.
Output: 1-2 tokens → "UK" or "EN".
Timeout: 150ms — if no response, fall back to Layer 2 result.
Run in separate thread, communicate via `std::future`.

---

## Binary File Formats

### `dictionary.bin`
4-byte magic `CLAV`, 4-byte LE entry count, then a flat open-addressing hash table.
Each slot: 8-byte xxh3_64 hash of the lowercase UTF-8 word, or `0x0000000000000000` for empty.
Load factor ≤ 0.7 — table size is `ceil(count / 0.7)` rounded to next power of 2.
Lookup: hash the query, probe linearly until match or empty slot.
Build tool: `tools/build_dict.py`.

### `keyboard_map.bin`
4-byte magic `KMAP`, 4-byte LE entry count, then pairs of:
`[4-byte UTF-8 codepoint (source)] [4-byte UTF-8 codepoint (target)]`.
Bidirectional — file contains both directions (EN→UK and UK→EN) in one blob.
Entries sorted by source codepoint for binary search.
Build tool: `tools/build_keymap.py` (reads `data/keyboard_maps/*.json`).

### `ngram.bin`
4-byte magic `NGRM`, 4-byte LE ngram count, 1-byte ngram size (3, 4, or 5).
Then per entry: `[N bytes UTF-8 ngram] [4-byte float LE log-probability]`.
Sorted alphabetically for binary search.
Build tool: `tools/build_ngram.py`.

---

## Thread Model

`clavid` runs 3 threads:

```
┌─────────────────┐     lock-free ring buffer     ┌──────────────────┐
│  Hook Thread    │ ──────────────────────────────▶│  Main Thread     │
│  (libuiohook)   │     KeyEvent structs           │  (event loop)    │
│  — captures HW  │                                │  — Layer 1 + 2   │
│    key events   │                                │  — undo stack    │
└─────────────────┘                                │  — toast trigger │
                                                   │  — IPC server    │
                                                   └───────┬──────────┘
                                                           │ std::future
                                                           ▼
                                                   ┌──────────────────┐
                                                   │  LLM Thread      │
                                                   │  (lazy-started)  │
                                                   │  — llama.cpp     │
                                                   │  — Layer 3 only  │
                                                   └──────────────────┘
```

- **Hook Thread**: owned by libuiohook. Pushes `KeyEvent` structs into a single-producer
  single-consumer lock-free ring buffer (capacity: 256 events). If full, drop oldest.
- **Main Thread**: polls ring buffer, runs detection pipeline, triggers platform switcher
  and toast. Also runs the IPC socket server (non-blocking `epoll`/`kqueue`/`IOCP`).
- **LLM Thread**: created on first Layer 3 call, stays alive until daemon exit.
  Receives work via `std::promise<LLMRequest>`, returns result via `std::future<LLMResult>`.

**Ownership rules:**
- Dictionary and layout map data are loaded once at startup, then **read-only** — no mutex needed.
- Undo stack is only accessed from Main Thread — no synchronization.
- Config reload (SIGHUP) happens on Main Thread — sets an atomic flag, Main Thread
  reloads packs on next loop iteration.

---

## Daemon Lifecycle

### Linux (systemd user unit)

```ini
# deploy/linux/clavi.service
[Unit]
Description=Clavi keyboard layout switcher
After=graphical-session.target

[Service]
Type=simple
ExecStart=%h/.local/bin/clavid
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
RestartSec=3

[Install]
WantedBy=default.target
```

Install: `systemctl --user enable --now clavi.service`

### macOS (launchd)

```xml
<!-- deploy/macos/dev.clavi.daemon.plist -->
<plist version="1.0">
<dict>
    <key>Label</key><string>dev.clavi.daemon</string>
    <key>ProgramArguments</key><array><string>/usr/local/bin/clavid</string></array>
    <key>RunAtLoad</key><true/>
    <key>KeepAlive</key><true/>
</dict>
</plist>
```

Install: `cp dev.clavi.daemon.plist ~/Library/LaunchAgents/ && launchctl load ~/Library/LaunchAgents/dev.clavi.daemon.plist`

### Windows

Use NSSM or register as a startup entry via registry
(`HKCU\Software\Microsoft\Windows\CurrentVersion\Run`).
For v1.0, simple startup entry is sufficient. Windows Service wrapper is v2.0.

### Signal handling (POSIX)

| Signal | Action |
|--------|--------|
| `SIGTERM` | Graceful shutdown: unhook keyboard, flush undo stack, close sockets, exit 0 |
| `SIGINT` | Same as SIGTERM |
| `SIGHUP` | Hot-reload config and language packs without restarting |
| `SIGUSR1` | Dump diagnostics to `~/.local/share/clavi/diag.log` (no keystroke data) |

On Windows: use `SetConsoleCtrlHandler` for Ctrl+C / service stop.

---

## Toast / Overlay System

Platform-native toasts, no heavy GUI framework:

| Platform | Implementation | Notes |
|----------|---------------|-------|
| Linux | `libnotify` (D-Bus `org.freedesktop.Notifications`) | Falls back to `notify-send` CLI |
| macOS | `NSUserNotificationCenter` (< 10.14) / `UNUserNotificationCenter` (≥ 10.14) | Requires bundle ID |
| Windows | `WinToast` library (COM-based) | Lightweight, header-only |

Toast content: `"Clavi: switched to {locale_name} → [Ctrl+Z to undo]"`
Duration: 3 seconds, non-interactive (click dismisses).

For **translit ghost text overlay** (v1.5): use a transparent always-on-top window
positioned above the caret. Dear ImGui with transparent background for cross-platform
consistency, or platform-native overlay (more work, better integration).

---

## Diagnostic Logging

**What CAN be logged** (opt-in via `config.toml`, disabled by default):
- Lifecycle events: `"daemon started"`, `"pack loaded: uk v1.0.0"`, `"daemon stopped"`
- Detection decisions: `"Layer 1 match: SwitchAndRetype → uk"` (NO actual text content)
- Performance: `"Layer 2 took 2.3ms"`, `"Layer 3 timeout after 150ms"`
- Errors: `"failed to load pack: file not found"`, `"libuiohook init failed"`

**What MUST NEVER be logged:**
- Actual keystrokes or typed characters
- Dictionary words that were matched
- Remapped text content
- Any buffer contents

Log destination: `~/.local/share/clavi/clavi.log`, max 5MB, rotate 3 files.
Log format: `[2025-01-15T14:30:00Z] [INFO] pack loaded: uk v1.0.0`

---

## Config Format — TOML Only

All config uses TOML (parsed via `toml++`). No YAML anywhere.

### Main config: `~/.config/clavi/config.toml`

```toml
[general]
enabled = true
active_pair = ["uk", "en"]    # primary language pair
min_word_length = 3            # never switch on words shorter than this

[hotkeys]
toggle = "Ctrl+Shift+Space"    # enable/disable Clavi
undo = "Ctrl+Z"                # revert last Clavi action

[detection]
layer2_threshold = 0.75        # n-gram confidence threshold
layer3_timeout_ms = 150        # LLM timeout
layer3_enabled = false         # opt-in (requires model download)

[logging]
enabled = false
level = "info"                 # debug | info | warn | error
```

### Exclusions: `~/.config/clavi/exclusions.toml`

```toml
[words]
skip = ["git", "npm", "sudo", "grep", "awk"]    # never remap these

[apps]
skip = ["terminal", "code", "idea"]               # disable Clavi in these apps
match = "substring"                                # "exact" | "substring" | "regex"
```

---

## UX Principles — Critical, Never Compromise

1. **Always show before acting.** Never silently replace text.
   - Show a small toast: `"Switched to Укр → [undo]"` for 3 seconds
   - Translit mode: show ghost text ABOVE input field, Tab to confirm
   - Bridge mode: show preview, user confirms before send

2. **Undo stack is mandatory from day one.**
   - One hotkey (default: `Ctrl+Z` or configurable) reverts last Clavi action
   - Store: {original_text, switched_text, layout_before, layout_after}
   - Keep last 10 actions

3. **Never switch on short words (≤2 chars).** Too many false positives.

4. **Exclusion list.** User can add words/apps that Clavi should never touch.
   Config: `~/.config/clavi/exclusions.toml`

---

## Performance Targets

| Metric | Target | Notes |
|--------|--------|-------|
| Layer 1 latency | < 1ms | Dictionary lookup + remap |
| Layer 2 latency | 1–3ms | N-gram scoring |
| Layer 3 latency | 20–80ms | LLM inference, 150ms hard timeout |
| Daemon startup | < 500ms | Without LLM model loading |
| LLM cold load | < 3s | First Layer 3 call only, then stays in RAM |
| RAM (no LLM) | < 30MB | Core + 2 dictionaries + n-gram model |
| RAM (with LLM) | < 500MB | Qwen2-0.5B q4_K_M ≈ 400MB |
| CPU idle | < 0.5% | When user is not typing |
| Binary size | < 5MB | clavid without LLM model |
| Pack size | < 10MB | Per language pack (dictionary + ngram + keymap) |

---

## Wayland Compatibility

`libuiohook` has limited Wayland support — it relies on X11 (`XRecord` / `XTest`)
which requires `XWayland` or fails entirely on pure Wayland compositors.

### Strategy

1. **v1.0**: Target X11 and XWayland. Document that pure Wayland (e.g. GNOME on
   Wayland without XWayland) is not supported yet.
2. **v1.1**: Implement `libei` (Emulated Input) backend as alternative to libuiohook
   on Wayland. `libei` is supported by GNOME 45+ and KDE 6.0+.
3. **Fallback detection**: At daemon startup, check `$XDG_SESSION_TYPE`:
   - `x11` → use libuiohook
   - `wayland` → check if XWayland is available, warn if not
   - Log: `"Wayland detected, XWayland fallback active"` or
     `"Pure Wayland detected, keyboard hook unavailable"`

### Wayland-specific files (v1.1+)

```
daemon/src/platform/
├── hook_x11.cpp        # libuiohook wrapper (existing)
├── hook_wayland.cpp    # libei-based hook (v1.1)
└── hook_factory.cpp    # runtime selection based on session type
```

---

## Feature Roadmap

### v1.0 — Core Switcher (start here)
- [ ] `libclavi-core`: layout_map + dictionary (Layer 1 + 2)
- [ ] `clavid` daemon with libuiohook
- [ ] Platform switcher: Linux first, then Mac, then Windows
- [ ] System tray icon
- [ ] Hotkey to enable/disable
- [ ] Undo hotkey
- [ ] Config file: active language pair, hotkeys, exclusions
- [ ] Ukr ↔ Eng pack
- [ ] systemd / launchd / Windows startup integration
- [ ] CI pipeline (GitHub Actions: build + test on 3 platforms)
- [ ] Diagnostic logging (non-content)

### v1.1 — Wayland + Polish
- [ ] `libei` backend for pure Wayland
- [ ] Hook factory (runtime X11/Wayland selection)
- [ ] Fuzzing harness for detector and layout_map

### v1.5 — Translit Input Mode
- [ ] Ghost text overlay above input fields
- [ ] KMU 2010 transliteration table
- [ ] Custom translit map via TOML (user can override)
- [ ] Tab to confirm, Esc to cancel

### v2.0 — LLM Layer + Bridge Mode
- [ ] Layer 3 detector (Qwen2-0.5B via llama.cpp)
- [ ] Bridge mode: type in language A, send in language B
- [ ] Preview before send
- [ ] More language packs (ES, FR, DE, KO...)

### v2.5 — Browser Extension
- [ ] Chrome/Firefox extension
- [ ] Communicates with clavid via localhost
- [ ] Textarea hook for translit ghost text
- [ ] Incoming translit → Cyrillic annotation (reader mode)

---

## Language Pack Format

Each pack lives in `packs/{locale}/pack.toml`:

```toml
[pack]
locale = "uk"
name = "Ukrainian"
version = "1.0.0"
author = "Clavi Project"

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

**`pack_loader.cpp` must contain:**
```cpp
// HARDCODED — do not make this configurable
static constexpr std::array<std::string_view, 1> BLOCKED_LOCALES = {"ru"};

bool PackLoader::is_allowed(std::string_view locale) noexcept {
    return std::ranges::none_of(BLOCKED_LOCALES,
        [&](auto blocked) { return locale == blocked; });
}
```

---

## Testing Strategy

### Unit tests (Catch2)

Every core module has a corresponding test file in `tests/unit/`.
Critical test cases:

| Test | What it verifies |
|------|-----------------|
| `test_pack_loader: ru_rejected` | `pack_loader.is_allowed("ru")` returns false — **must never break** |
| `test_layout_map: en_to_uk_roundtrip` | Remap EN→UK→EN produces original |
| `test_layout_map: all_keys_mapped` | Every printable key has a mapping |
| `test_dictionary: known_words` | "привіт", "hello" found after loading |
| `test_dictionary: unknown_words` | Random garbage not found |
| `test_detector: ghbdtn_switches` | The acceptance criteria from "Start Here" |
| `test_detector: short_word_skip` | Words ≤2 chars → NoAction always |
| `test_detector: ambiguous_word` | Word in both dicts → NoAction |
| `test_undo_stack: capacity_10` | 11th push evicts oldest |
| `test_config: missing_file` | Returns defaults, no crash |

### Integration tests

- `test_detector_e2e`: load real packs, process sequences of words, verify correct
  switch/no-switch decisions across a realistic typing session.
- `test_daemon_ipc`: start daemon, connect via socket, send mock events, verify responses.

### Fuzz testing

- `fuzz_detector`: random UTF-8 byte sequences as input — must never crash, never UB.
- `fuzz_layout_map`: random codepoints — must return valid UTF-8 or empty.
- Run via `clang -fsanitize=fuzzer` with ASan + UBSan.
- CI: weekly scheduled run, 10 minutes per target.

### Coverage target

Aim for ≥ 90% line coverage on `core/`. Measure with `llvm-cov` / `gcov`.

---

## CI/CD (GitHub Actions)

### `ci.yml` — on every push and PR

```yaml
matrix:
  os: [ubuntu-24.04, macos-14, windows-2022]
  build_type: [Debug, Release]
steps:
  - checkout with submodules
  - install deps (libnotify-dev on Linux, etc.)
  - cmake --preset {os}-{build_type}
  - cmake --build build/
  - ctest --test-dir build/ --output-on-failure
  - clang-tidy (Linux Debug only)
  - clang-format check (Linux Debug only)
```

### `release.yml` — on tag `v*`

- Build Release on all 3 platforms
- CPack: `.deb`, `.rpm`, `.pkg`, `.msi`
- Upload to GitHub Releases

### `fuzz.yml` — weekly cron

- Linux only, build with `-fsanitize=fuzzer,address,undefined`
- Run each fuzz target for 10 minutes
- Upload crash artifacts if found

---

## Start Here — First Task

Build `libclavi-core` with Layer 1 detection only.

**Acceptance criteria:**
```cpp
// This test must pass:
ClaviDetector detector;
detector.load_pack("packs/uk");
detector.load_pack("packs/en");

auto result = detector.analyze("ghbdtn");  // typed on EN layout
assert(result.action == Action::SwitchAndRetype);
assert(result.target_locale == "uk");
assert(result.corrected_text == "привіт");

auto result2 = detector.analyze("hello");
assert(result2.action == Action::NoAction);

auto result3 = detector.analyze("привіт");
assert(result3.action == Action::NoAction);
```

Start with `core/src/layout_map.cpp` — the Ukrainian ↔ English key mapping table.
Then `core/src/dictionary.cpp` — binary hash-set loader.
Then wire them together in `core/src/detector.cpp`.

---

## Build Instructions

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/yourname/clavi

# Configure
cmake --preset linux-debug   # or macos-debug, windows-debug

# Build
cmake --build build/

# Run tests
ctest --test-dir build/

# Build tools (generate binary data from raw sources)
python3 tools/build_keymap.py data/keyboard_maps/uk_qwerty.json data/keyboard_maps/en_qwerty.json packs/uk/keyboard_map.bin
python3 tools/build_keymap.py data/keyboard_maps/en_qwerty.json data/keyboard_maps/uk_qwerty.json packs/en/keyboard_map.bin
python3 tools/build_dict.py data/wordlists/uk_words.txt packs/uk/dictionary.bin
python3 tools/build_dict.py data/wordlists/en_words.txt packs/en/dictionary.bin
python3 tools/build_ngram.py data/wordlists/uk_words.txt packs/uk/ngram.bin --ngram-size 3
```

---

## Code Style

- C++20, no exceptions in hot path (use `std::expected` or error codes)
- `[[nodiscard]]` on all functions returning results
- Header-only templates in `include/`, implementations in `src/`
- No RTTI (`-fno-rtti`)
- All strings as `std::string_view` in hot path, no copies
- Unit tests with Catch2 (add as submodule or FetchContent)
- Clang-format with Google style, 100 col limit
- Clang-tidy enabled in CI with `modernize-*`, `bugprone-*`, `performance-*` checks

---

## Notes for Claude Code

- When implementing `switcher_linux.cpp`, use `xdotool key` for layout switching
  and `xdotool type` for retyping. Check if `ibus` is available as fallback.
- When implementing `switcher_mac.cpp`, use `TISSelectInputSource` from Carbon framework.
- When implementing `switcher_win.cpp`, use `PostMessage(HWND_BROADCAST, WM_INPUTLANGCHANGEREQUEST, ...)`
  combined with `SendInput` for retyping.
- The `dictionary.bin` format: see "Binary File Formats" section above.
- The `keyboard_map.bin` format: see "Binary File Formats" section above.
- For `libuiohook` callbacks: keyboard events arrive on a separate thread.
  Use a lock-free queue (`std::atomic` + ring buffer) to pass events to the main thread.
  See "Thread Model" section for full architecture.
- Ukrainian keyboard map reference: https://kbdlayout.info/kbdur1
- When handling ambiguous words (exist in both dicts), return `NoAction` — see
  "Ambiguous words" in Detection Algorithm section.
- All config files are TOML — do NOT use YAML anywhere.
- Toast implementation: use platform-native approach (see "Toast / Overlay System").
- Wayland: check `$XDG_SESSION_TYPE` at startup, warn if pure Wayland without XWayland.