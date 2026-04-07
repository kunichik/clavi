#include "clavi/version.hpp"
#include "clavi/hook.hpp"
#include "clavi/platform/switcher.hpp"
#include "clavi/platform/toast.hpp"
#include "clavi/platform/tray.hpp"
#include "clavi/detector.hpp"
#include "clavi/config.hpp"
#include "clavi/logger.hpp"
#include "clavi/undo_stack.hpp"
#include "clavi/utf8_utils.hpp"

#ifdef _MSC_VER
#pragma warning(disable : 4996) // getenv deprecation
#endif

#include <atomic>
#include <csignal>
#include <cstdio>
#include <filesystem>
#include <string>

namespace {

// ── Globals ──────────────────────────────────────────────────────────────────

static std::atomic<bool> g_quit{false};
static clavi::Hook       g_hook;

// ── Signal handling ──────────────────────────────────────────────────────────

extern "C" void signal_handler(int /*sig*/) {
    g_quit.store(true);
    g_hook.stop();
}

// ── Config path helpers ──────────────────────────────────────────────────────

std::filesystem::path config_dir() {
    namespace fs = std::filesystem;
#if defined(_WIN32)
    const char* appdata = std::getenv("APPDATA");
    return appdata ? fs::path(appdata) / "clavi" : fs::path(".");
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    return home ? fs::path(home) / "Library" / "Application Support" / "clavi"
                : fs::path(".");
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) return fs::path(xdg) / "clavi";
    const char* home = std::getenv("HOME");
    return home ? fs::path(home) / ".config" / "clavi" : fs::path(".");
#endif
}

std::filesystem::path packs_dir() {
    namespace fs = std::filesystem;
#if defined(_WIN32)
    const char* appdata = std::getenv("PROGRAMDATA");
    return appdata ? fs::path(appdata) / "clavi" / "packs"
                   : fs::path(".") / "packs";
#elif defined(__APPLE__)
    return fs::path("/Library/Application Support/clavi/packs");
#else
    return fs::path("/usr/share/clavi/packs");
#endif
}

} // namespace

int main(int argc, char* argv[]) {
    namespace fs = std::filesystem;

    // ── Parse minimal args ────────────────────────────────────────────────────
    bool verbose = false;
    std::string cli_mode;          // overrides config general.mode
    std::string cli_translit;      // overrides config general.translit_locale
    std::string cli_packs;         // overrides pack directory
    std::string cli_config;        // overrides config directory

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--version") {
            std::printf("clavid %s (%s)\n", clavi::VERSION, clavi::GIT_HASH);
            return 0;
        } else if (arg == "--help" || arg == "-h") {
            std::puts(
                "Usage: clavid [options]\n"
                "\n"
                "Options:\n"
                "  -v, --verbose              Print events to stdout\n"
                "  --version                  Print version and exit\n"
                "  --mode <mode>              Override config mode:\n"
                "                               detection (default)\n"
                "                               bridge (always-on translit)\n"
                "  --translit-locale <locale> Override translit target locale (e.g. uk)\n"
                "  --packs <dir>              Override language packs directory\n"
                "  --config <dir>             Override config directory\n"
                "  -h, --help                 Show this help"
            );
            return 0;
        } else if (arg == "--mode" && i + 1 < argc) {
            cli_mode = argv[++i];
        } else if (arg == "--translit-locale" && i + 1 < argc) {
            cli_translit = argv[++i];
        } else if (arg == "--packs" && i + 1 < argc) {
            cli_packs = argv[++i];
        } else if (arg == "--config" && i + 1 < argc) {
            cli_config = argv[++i];
        }
    }

    // ── Signal handlers ───────────────────────────────────────────────────────
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // ── Load config ───────────────────────────────────────────────────────────────
    const fs::path cfg_dir  = cli_config.empty() ? config_dir() : fs::path(cli_config);
    const fs::path cfg_path = cfg_dir / "config.toml";
    const fs::path excl_path = cfg_dir / "exclusions.toml";

    clavi::Config cfg = clavi::Config::load(cfg_path.string(), excl_path.string());

    // Apply CLI overrides (take precedence over config file)
    if (!cli_mode.empty())     cfg.general.mode             = cli_mode;
    if (!cli_translit.empty()) cfg.general.translit_locale  = cli_translit;

    // Validate config
    const auto errors = cfg.validate();
    if (!errors.empty()) {
        for (const auto& err : errors)
            std::fprintf(stderr, "[clavid] config error: %s\n", err.c_str());
        return 1;
    }

    // ── Diagnostic logger (non-content, privacy-safe) ────────────────────────
    clavi::Logger logger;
    if (cfg.logging.enabled) {
        const auto log_level = clavi::parse_log_level(cfg.logging.level);
        const std::string log_path = clavi::Logger::default_log_path();
        if (!logger.open(log_path, log_level)) {
            std::fprintf(stderr, "[clavid] warning: cannot open log: %s\n",
                         log_path.c_str());
        }
    }

    if (verbose) {
        std::printf("[clavid] config dir: %s\n", cfg_dir.string().c_str());
        std::printf("[clavid] enabled: %s\n", cfg.general.enabled ? "true" : "false");
    }
    logger.info("daemon started");

#if !defined(_WIN32) && !defined(__APPLE__)
    // Wayland session detection (Linux only)
    if (const char* session = std::getenv("XDG_SESSION_TYPE")) {
        if (std::string(session) == "wayland") {
            // libuiohook needs X11 — check if XWayland is available
            const bool have_display = std::getenv("DISPLAY") != nullptr;
            if (have_display) {
                logger.info("Wayland detected, XWayland fallback active");
                if (verbose) std::puts("[clavid] Wayland + XWayland detected");
            } else {
                logger.warn("pure Wayland detected, keyboard hook unavailable");
                std::fputs("[clavid] WARNING: pure Wayland without XWayland — "
                           "keyboard hook will not work\n", stderr);
            }
        }
    }
#endif

    if (!cfg.general.enabled) {
        if (verbose) std::puts("[clavid] disabled via config — exiting");
        logger.info("disabled via config -- exiting");
        return 0;
    }

    // ── Load language packs ───────────────────────────────────────────────────
    clavi::Detector detector;
    const fs::path pdir = cli_packs.empty() ? packs_dir() : fs::path(cli_packs);

    for (const auto& locale : cfg.general.active_pair) {
        const fs::path pack_path = pdir / locale;
        if (!fs::is_directory(pack_path)) {
            if (verbose)
                std::fprintf(stderr, "[clavid] pack not found: %s\n",
                             pack_path.string().c_str());
            logger.warn(std::string("pack not found: ") + locale);
            continue;
        }
        if (detector.load_pack(pack_path.string())) {
            if (verbose) std::printf("[clavid] loaded pack: %s\n", locale.c_str());
            logger.info(std::string("pack loaded: ") + locale);
        } else {
            std::fprintf(stderr, "[clavid] failed to load pack: %s\n",
                         locale.c_str());
            logger.error(std::string("failed to load pack: ") + locale);
        }
    }

    if (detector.pack_count() < 2) {
        std::fprintf(stderr,
            "[clavid] need at least 2 language packs to operate "
            "(found %zu). Check packs directory: %s\n",
            detector.pack_count(), pdir.string().c_str());
        logger.error("insufficient packs -- exiting");
        return 1;
    }

    // ── Platform services ─────────────────────────────────────────────────────
    auto switcher = clavi::ISwitcher::create();
    auto toast    = clavi::IToast::create();
    auto tray     = clavi::ITray::create();
    clavi::UndoStack undo_stack;

    // Currently active locale (we start with the first in the pair)
    std::string current_locale = cfg.general.active_pair.empty()
                                     ? "en"
                                     : cfg.general.active_pair[0];
    std::atomic<bool> enabled{true};
    const bool bridge_mode = (cfg.general.mode == "bridge");
    std::atomic<bool> translit_active{bridge_mode}; // bridge=always-on

    // Target locale for translit mode (configurable, defaults to "uk")
    const std::string translit_locale = cfg.general.translit_locale;

    // ── Build callbacks ───────────────────────────────────────────────────────
    clavi::HookCallbacks cbs;

    cbs.on_word_commit = [&](const std::string& word) {
        if (!enabled.load()) return;

        // Translit mode: convert Latin input to target script and retype
        if (translit_active.load()) {
            const std::string converted =
                detector.transliterate(word, translit_locale);
            if (!converted.empty() && converted != word) {
                const std::size_t char_count = clavi::utf8::count(word);
                switcher->retype(char_count, converted);
                logger.debug("translit retype");
                if (verbose)
                    std::printf("[clavid] translit: '%s' -> '%s'\n",
                                word.c_str(), converted.c_str());
            }
            return;
        }

        // Check exclusion list
        const std::string lower = clavi::utf8::to_lower(word);
        for (const auto& skip : cfg.exclusions.skip_words) {
            if (cfg.exclusions.match == "exact") {
                if (lower == clavi::utf8::to_lower(skip)) return;
            } else {
                if (lower.find(clavi::utf8::to_lower(skip)) != std::string::npos)
                    return;
            }
        }

        const clavi::DetectionResult result = detector.analyze(word);
        if (result.action != clavi::Action::SwitchAndRetype) return;

        const std::size_t char_count = clavi::utf8::count(word);

        // Switch layout
        if (!switcher->switch_layout(result.target_locale)) {
            if (verbose)
                std::fprintf(stderr, "[clavid] switch_layout(%s) failed\n",
                             result.target_locale.c_str());
            logger.warn(std::string("switch_layout failed: ") +
                        result.target_locale);
            return;
        }

        // Retype
        switcher->retype(char_count, result.corrected_text);

        // Save to undo stack
        undo_stack.push({word, result.corrected_text,
                         current_locale, result.target_locale});
        current_locale = result.target_locale;

        // Toast notification
        const std::string toast_body =
            result.corrected_text + " \xE2\x86\x90 " + word; // "corrected ← original"
        toast->show("Clavi", toast_body, 2000);

        // PRIVACY: log locale change only, never the typed/remapped content
        logger.info(std::string("Layer 1: SwitchAndRetype -> ") +
                    result.target_locale);
        if (verbose)
            std::printf("[clavid] switched: '%s' -> '%s' (%s)\n",
                        word.c_str(), result.corrected_text.c_str(),
                        result.target_locale.c_str());
    };

    cbs.on_undo = [&]() {
        const auto entry = undo_stack.pop();
        if (!entry) return;

        // Switch back
        (void)switcher->switch_layout(entry->locale_before);
        const std::size_t retype_count = clavi::utf8::count(entry->switched_text);
        switcher->retype(retype_count, entry->original_text);

        current_locale = entry->locale_before;
        toast->show("Clavi", std::string("undo: ") + entry->original_text, 1500);

        logger.info(std::string("undo -> ") + entry->locale_before);
        if (verbose)
            std::printf("[clavid] undo: '%s' -> '%s'\n",
                        entry->switched_text.c_str(),
                        entry->original_text.c_str());
    };

    cbs.on_translit_toggle = [&]() {
        const bool now = !translit_active.load();
        translit_active.store(now);
        toast->show("Clavi", now ? "translit ON" : "translit OFF", 1000);
        logger.info(now ? "translit ON" : "translit OFF");
        if (verbose)
            std::printf("[clavid] translit %s\n", now ? "ON" : "OFF");
    };

    cbs.on_toggle = [&]() {
        const bool now = !enabled.load();
        enabled.store(now);
        tray->set_enabled(now);
        toast->show("Clavi", now ? "enabled" : "paused", 1000);
        logger.info(now ? "enabled" : "paused");
        if (verbose) std::printf("[clavid] %s\n", now ? "enabled" : "paused");
    };

    // ── System tray icon ───────────────────────────────────────────────────────
    clavi::ITray::Callbacks tray_cbs;
    tray_cbs.on_toggle = cbs.on_toggle; // reuse same toggle logic
    tray_cbs.on_quit = [&]() {
        logger.info("quit via tray");
        g_quit.store(true);
        g_hook.stop();
    };
    tray->init(std::move(tray_cbs));

    // ── Start hook (blocks until signal) ─────────────────────────────────────
    if (bridge_mode) {
        logger.info(std::string("bridge mode active -> ") + translit_locale);
        if (verbose) std::printf("[clavid] bridge mode -> %s\n",
                                 translit_locale.c_str());
    }
    if (verbose) std::puts("[clavid] hook started");
    logger.info("hook started");

    g_hook.run(std::move(cbs));

    tray->shutdown();
    logger.info("daemon stopped");
    if (verbose) std::puts("[clavid] exiting");
    return 0;
}
