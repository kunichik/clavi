#include <catch2/catch_test_macros.hpp>
#include "clavi/logger.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string read_all(const std::string& path) {
    std::ifstream f(path);
    return {std::istreambuf_iterator<char>(f), {}};
}

static void cleanup(const std::string& path) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::remove(path, ec);
    fs::remove(path + ".1", ec);
    fs::remove(path + ".2", ec);
    fs::remove(path + ".3", ec);
}

// ── Tests ─────────────────────────────────────────────────────────────────────

TEST_CASE("Logger: default state", "[logger]") {
    clavi::Logger logger;
    REQUIRE(!logger.is_open());
    // log when closed is a no-op (no crash)
    logger.info("should not crash");
}

TEST_CASE("Logger: open and write", "[logger]") {
    const std::string path = "test_logger_basic.log";
    cleanup(path);

    {
        clavi::Logger logger;
        REQUIRE(logger.open(path));
        REQUIRE(logger.is_open());
        logger.info("hello world");
        logger.error("something bad");
    } // destructor closes

    const std::string content = read_all(path);
    REQUIRE(content.find("[INFO] hello world") != std::string::npos);
    REQUIRE(content.find("[ERROR] something bad") != std::string::npos);

    cleanup(path);
}

TEST_CASE("Logger: level filtering", "[logger]") {
    const std::string path = "test_logger_level.log";
    cleanup(path);

    {
        clavi::Logger logger;
        REQUIRE(logger.open(path, clavi::LogLevel::Warn));
        logger.debug("skip me");
        logger.info("skip me too");
        logger.warn("keep me");
        logger.error("keep me too");
    }

    const std::string content = read_all(path);
    REQUIRE(content.find("skip me") == std::string::npos);
    REQUIRE(content.find("[WARN] keep me") != std::string::npos);
    REQUIRE(content.find("[ERROR] keep me too") != std::string::npos);

    cleanup(path);
}

TEST_CASE("Logger: ISO 8601 timestamp format", "[logger]") {
    const std::string path = "test_logger_ts.log";
    cleanup(path);

    {
        clavi::Logger logger;
        REQUIRE(logger.open(path, clavi::LogLevel::Info));
        logger.info("timestamp check");
    }

    const std::string content = read_all(path);
    // Format: [YYYY-MM-DDTHH:MM:SSZ] [INFO] ...
    // Check that the line starts with [20
    REQUIRE(content.size() > 22);
    REQUIRE(content[0] == '[');
    REQUIRE(content[1] == '2');
    REQUIRE(content[2] == '0');
    REQUIRE(content.find('T') != std::string::npos);
    REQUIRE(content.find("Z]") != std::string::npos);

    cleanup(path);
}

TEST_CASE("parse_log_level", "[logger]") {
    REQUIRE(clavi::parse_log_level("debug") == clavi::LogLevel::Debug);
    REQUIRE(clavi::parse_log_level("info") == clavi::LogLevel::Info);
    REQUIRE(clavi::parse_log_level("warn") == clavi::LogLevel::Warn);
    REQUIRE(clavi::parse_log_level("error") == clavi::LogLevel::Error);
    REQUIRE(clavi::parse_log_level("unknown") == clavi::LogLevel::Info);
    REQUIRE(clavi::parse_log_level("") == clavi::LogLevel::Info);
}

TEST_CASE("Logger: creates parent directories", "[logger]") {
    const std::string path = "test_logger_subdir/nested/test.log";
    cleanup(path);

    {
        clavi::Logger logger;
        REQUIRE(logger.open(path));
        logger.info("nested dir");
    }

    REQUIRE(std::filesystem::exists(path));
    const std::string content = read_all(path);
    REQUIRE(content.find("nested dir") != std::string::npos);

    // Cleanup
    std::error_code ec;
    std::filesystem::remove_all("test_logger_subdir", ec);
}

TEST_CASE("Logger: default_log_path is non-empty", "[logger]") {
    const std::string p = clavi::Logger::default_log_path();
    REQUIRE(!p.empty());
    REQUIRE(p.find("clavi") != std::string::npos);
}
