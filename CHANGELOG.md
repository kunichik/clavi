# Changelog

All notable changes to Clavi are documented in this file.

## [Unreleased]

### Added
- `--config <dir>` CLI flag to override config directory
- `Config::validate()` method with comprehensive field checks (mode, log level, thresholds, match type)
- 7 unit tests for config validation + 4 for `PackLoader::load_pack_info()`
- `PackLoader::load_pack_info()` — proper TOML parsing of `pack.toml` (replaces hacky text scan)
- `PackInfo` struct with locale, name, version, feature flags, file paths
- Wayland session type detection at startup (Linux only)
- `version.hpp` with structured `--version` output: `clavid 0.1.0 (dev)`
- Ukrainian apostrophe rule `'` → `ʼ` (U+02BC) in translit.toml
- CMakePresets: `linux-daemon`, `windows-daemon`, `linux-fuzz` presets
- CPack packaging config (DEB, RPM, DMG, NSIS, ZIP)
- `release.yml` CI workflow — auto-build packages on `v*` tag push
- `docs/config-example.toml` — fully annotated reference config
- `all_keys_mapped` test for LayoutMap (spec-required critical test)

### Fixed
- `en/pack.toml` missing `ngram.bin` reference
- `uk/pack.toml` bridge feature flag set to `true`
- Deploy: Linux systemd service targets `graphical-session.target`
- Deploy: macOS plist uses `SuccessfulExit=false` restart semantics

### Refactored
- `Detector::load_pack()` now uses `PackLoader::load_pack_info()` instead of manual text parsing

## [0.1.0] — 2026-04-07

### v2.0 — Bridge Mode
- **Bridge mode**: always-on translit via `mode = "bridge"` in config or `--mode bridge`
- CLI flags: `--mode`, `--translit-locale`, `--packs`
- `translit_locale` config field with TOML parsing

### v1.5 — Transliteration Input Mode
- **KMU 2010 transliteration** table (`packs/uk/translit.toml`)
- **Ctrl+T hotkey** to toggle translit mode in daemon
- Detector integration: `Detector::transliterate()` API
- Translit toast notifications on mode toggle
- 10 integration test cases for transliteration

### v1.1 — Fuzzing
- 3 libFuzzer targets: `fuzz_detector`, `fuzz_layout_map`, `fuzz_ngram_model`
- Weekly CI fuzzing workflow (`fuzz.yml`)
- clang-format CI extended to `fuzz/` directory

### v1.0 — Core Switcher
- **Layer 1**: deterministic dictionary lookup + keyboard remapping
- **Layer 2**: character n-gram statistical detection
- **System tray icon** with context menu (Pause / Enable / Quit) — Windows
- **libuiohook** keyboard hook with word boundary detection
- **WordBuffer**: accumulates typed characters, emits words on space/enter/punctuation
- **Hotkeys**: Ctrl+Shift+Space (toggle), Ctrl+Z (undo)
- **Undo stack**: 10-deep, restores original text and layout
- **Toast notifications**: platform-native (Windows shell notification)
- **Diagnostic logger**: file-based, rotating (5MB × 3), level-filtered, thread-safe
- **Config system**: TOML-based config + exclusions (per-word, per-app)
- **Language packs**: Ukrainian (uk) + English (en) with dictionary, keyboard map, n-gram model
- **Platform switchers**: Windows (`SendInput`), Linux (`xdotool`), macOS (`TIS`) stubs
- **CI pipeline**: GitHub Actions — build + test on Linux/macOS/Windows, clang-tidy, clang-format
- **Deploy files**: systemd unit, launchd plist, Windows NSSM XML

### Foundation
- `libclavi-core` static library: detector, layout_map, dictionary, translit, config, logger, undo_stack, pack_loader, utf8_utils, ngram_model
- `clavid` daemon executable
- Catch2 test suite: **95 test cases, 334 assertions**
- `xxHash` (xxh3_64) for dictionary and keyboard map hashing
- `toml++` for all configuration parsing
- Pack format: `pack.toml` + binary data files
- `PackLoader::is_allowed()` — hardcoded `"ru"` locale rejection
