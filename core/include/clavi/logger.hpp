#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>

namespace clavi {

enum class LogLevel : uint8_t {
    Debug = 0,
    Info  = 1,
    Warn  = 2,
    Error = 3,
};

// Parse log level string ("debug", "info", "warn", "error"). Returns Info on unknown.
[[nodiscard]] LogLevel parse_log_level(std::string_view s) noexcept;

// Thread-safe file logger with rotation.
// PRIVACY: This logger MUST NEVER receive actual keystroke content,
// dictionary words, or remapped text. Only lifecycle events, detection
// decisions (locale names only), performance metrics, and errors.
//
// Log destination: configurable path, default ~/.local/share/clavi/clavi.log
// Max file size: 5 MB, rotates up to 3 files (clavi.log.1, .2, .3)
// Format: [2025-01-15T14:30:00Z] [INFO] message
class Logger {
public:
    Logger() = default;
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Open log file. Creates parent directories if needed.
    // Returns false if the file cannot be opened.
    [[nodiscard]] bool open(std::string_view path, LogLevel min_level = LogLevel::Info);

    // Close the log file.
    void close() noexcept;

    [[nodiscard]] bool is_open() const noexcept { return file_ != nullptr; }

    // Log a message at the given level. Thread-safe.
    // Messages below min_level are silently dropped.
    void log(LogLevel level, std::string_view msg);

    // Convenience wrappers
    void debug(std::string_view msg) { log(LogLevel::Debug, msg); }
    void info(std::string_view msg)  { log(LogLevel::Info, msg); }
    void warn(std::string_view msg)  { log(LogLevel::Warn, msg); }
    void error(std::string_view msg) { log(LogLevel::Error, msg); }

    // Platform default log path
    [[nodiscard]] static std::string default_log_path();

private:
    static constexpr std::size_t MAX_FILE_SIZE = 5 * 1024 * 1024; // 5 MB
    static constexpr int MAX_ROTATIONS = 3;

    std::mutex mutex_;
    std::FILE* file_{nullptr};
    std::string path_;
    LogLevel min_level_{LogLevel::Info};
    std::size_t current_size_{0};

    void rotate();
    [[nodiscard]] static const char* level_tag(LogLevel level) noexcept;
    [[nodiscard]] static std::string timestamp_now();
};

} // namespace clavi
