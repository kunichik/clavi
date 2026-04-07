#include "clavi/logger.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

namespace clavi {

// ── Level parsing ────────────────────────────────────────────────────────────

LogLevel parse_log_level(std::string_view s) noexcept {
    if (s == "debug") return LogLevel::Debug;
    if (s == "info")  return LogLevel::Info;
    if (s == "warn")  return LogLevel::Warn;
    if (s == "error") return LogLevel::Error;
    return LogLevel::Info;
}

const char* Logger::level_tag(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "INFO";
}

// ── Timestamp ────────────────────────────────────────────────────────────────

std::string Logger::timestamp_now() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto tt = system_clock::to_time_t(now);

    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &tt);
#else
    gmtime_r(&tt, &utc);
#endif

    char buf[32]{};
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                  utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                  utc.tm_hour, utc.tm_min, utc.tm_sec);
    return buf;
}

// ── Default path ─────────────────────────────────────────────────────────────

std::string Logger::default_log_path() {
    namespace fs = std::filesystem;
#if defined(_WIN32)
    const char* appdata = std::getenv("LOCALAPPDATA");
    if (appdata) return (fs::path(appdata) / "clavi" / "clavi.log").string();
    return "clavi.log";
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    if (home)
        return (fs::path(home) / "Library" / "Logs" / "clavi" / "clavi.log").string();
    return "clavi.log";
#else
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg && *xdg)
        return (fs::path(xdg) / "clavi" / "clavi.log").string();
    const char* home = std::getenv("HOME");
    if (home)
        return (fs::path(home) / ".local" / "share" / "clavi" / "clavi.log").string();
    return "clavi.log";
#endif
}

// ── Open / Close ─────────────────────────────────────────────────────────────

bool Logger::open(std::string_view path, LogLevel min_level) {
    std::lock_guard lock(mutex_);
    if (file_) std::fclose(file_);

    path_ = std::string(path);
    min_level_ = min_level;

    // Create parent directories
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(fs::path(path_).parent_path(), ec);

    file_ = std::fopen(path_.c_str(), "ab");
    if (!file_) return false;

    // Determine current file size
    std::fseek(file_, 0, SEEK_END);
    current_size_ = static_cast<std::size_t>(std::ftell(file_));
    return true;
}

void Logger::close() noexcept {
    std::lock_guard lock(mutex_);
    if (file_) {
        std::fclose(file_);
        file_ = nullptr;
    }
}

Logger::~Logger() { close(); }

// ── Logging ──────────────────────────────────────────────────────────────────

void Logger::log(LogLevel level, std::string_view msg) {
    if (static_cast<uint8_t>(level) < static_cast<uint8_t>(min_level_)) return;

    std::lock_guard lock(mutex_);
    if (!file_) return;

    const std::string ts = timestamp_now();
    const char* tag = level_tag(level);

    // Format: [2025-01-15T14:30:00Z] [INFO] message\n
    const int n = std::fprintf(file_, "[%s] [%s] %.*s\n",
                               ts.c_str(), tag,
                               static_cast<int>(msg.size()), msg.data());
    if (n > 0) {
        current_size_ += static_cast<std::size_t>(n);
        std::fflush(file_);
    }

    if (current_size_ >= MAX_FILE_SIZE) {
        rotate();
    }
}

// ── Rotation ─────────────────────────────────────────────────────────────────

void Logger::rotate() {
    // Close current file
    if (file_) {
        std::fclose(file_);
        file_ = nullptr;
    }

    namespace fs = std::filesystem;
    std::error_code ec;

    // Rotate: .3 → delete, .2 → .3, .1 → .2, current → .1
    for (int i = MAX_ROTATIONS; i >= 1; --i) {
        const std::string src = (i == 1)
            ? path_
            : path_ + "." + std::to_string(i - 1);
        const std::string dst = path_ + "." + std::to_string(i);

        if (fs::exists(src, ec)) {
            if (i == MAX_ROTATIONS) fs::remove(dst, ec);
            fs::rename(src, dst, ec);
        }
    }

    // Reopen fresh log
    file_ = std::fopen(path_.c_str(), "wb");
    current_size_ = 0;
}

} // namespace clavi
