#include <catch2/catch_test_macros.hpp>
#include "clavi/text_fix_engine.hpp"

using clavi::TextFixEngine;

// ── EN typos ──────────────────────────────────────────────────────────────────

TEST_CASE("EN classic transpositions", "[text_fix]") {
    auto fix = TextFixEngine::check_word("teh");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "the");

    fix = TextFixEngine::check_word("adn");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "and");

    fix = TextFixEngine::check_word("yuo");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "you");
}

TEST_CASE("EN typo with capitalization preserved", "[text_fix]") {
    // "Teh" (sentence start) → "The"
    auto fix = TextFixEngine::check_word("Teh");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "The");
}

TEST_CASE("EN missing apostrophe", "[text_fix]") {
    auto fix = TextFixEngine::check_word("dont");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "don't");

    fix = TextFixEngine::check_word("wouldnt");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "wouldn't");
}

TEST_CASE("EN i-before-e violations", "[text_fix]") {
    auto fix = TextFixEngine::check_word("recieve");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "receive");

    fix = TextFixEngine::check_word("freind");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "friend");
}

TEST_CASE("EN common misspellings", "[text_fix]") {
    auto fix = TextFixEngine::check_word("definately");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "definitely");

    fix = TextFixEngine::check_word("seperate");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "separate");

    fix = TextFixEngine::check_word("occured");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "occurred");
}

// ── UK typos ──────────────────────────────────────────────────────────────────

TEST_CASE("UK translit artifact", "[text_fix]") {
    auto fix = TextFixEngine::check_word("pryvit");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == u8"привіт");
}

TEST_CASE("UK missing apostrophe", "[text_fix]") {
    auto fix = TextFixEngine::check_word(u8"памятати");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == u8"пам'ятати");

    fix = TextFixEngine::check_word(u8"компютер");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == u8"комп'ютер");

    fix = TextFixEngine::check_word(u8"пять");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == u8"п'ять");
}

TEST_CASE("UK missing soft sign", "[text_fix]") {
    auto fix = TextFixEngine::check_word(u8"будте");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == u8"будьте");
}

// ── Repeated characters ───────────────────────────────────────────────────────

TEST_CASE("Repeated chars: 3+ reduced to 1", "[text_fix]") {
    auto fix = TextFixEngine::check_word("hellllo");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "hello");

    fix = TextFixEngine::check_word("sooooo");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "so");
}

TEST_CASE("Repeated chars: 2 left unchanged", "[text_fix]") {
    // "ee" in "keen" is legitimate — 2 repeats, not 3+
    auto fix = TextFixEngine::check_word("keen");
    REQUIRE_FALSE(fix.has_value());
}

TEST_CASE("Repeated chars: too short (< 4 bytes)", "[text_fix]") {
    auto fix = TextFixEngine::check_word("aaa");
    REQUIRE_FALSE(fix.has_value());  // length < 4 → skip
}

// ── Accidental CapsLock ───────────────────────────────────────────────────────

TEST_CASE("Accidental caps: HEllo → Hello", "[text_fix]") {
    auto fix = TextFixEngine::check_word("HEllo");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "Hello");
}

TEST_CASE("Accidental caps: SHift → Shift", "[text_fix]") {
    auto fix = TextFixEngine::check_word("SHift");
    REQUIRE(fix.has_value());
    REQUIRE(fix->corrected == "Shift");
}

TEST_CASE("Accidental caps: all-caps abbreviations not touched", "[text_fix]") {
    // HTTP, NATO, API etc. — all uppercase → don't fix
    REQUIRE_FALSE(TextFixEngine::check_word("HTTP").has_value());
    REQUIRE_FALSE(TextFixEngine::check_word("NATO").has_value());
    REQUIRE_FALSE(TextFixEngine::check_word("API").has_value());
}

TEST_CASE("Accidental caps: normal Title Case not touched", "[text_fix]") {
    REQUIRE_FALSE(TextFixEngine::check_word("Hello").has_value());
    REQUIRE_FALSE(TextFixEngine::check_word("Kyiv").has_value());
}

// ── No false positives on correct words ───────────────────────────────────────

TEST_CASE("Correct EN words untouched", "[text_fix]") {
    REQUIRE_FALSE(TextFixEngine::check_word("the").has_value());
    REQUIRE_FALSE(TextFixEngine::check_word("and").has_value());
    REQUIRE_FALSE(TextFixEngine::check_word("receive").has_value());
    REQUIRE_FALSE(TextFixEngine::check_word("definitely").has_value());
    REQUIRE_FALSE(TextFixEngine::check_word("tomorrow").has_value());
}

TEST_CASE("Short words untouched", "[text_fix]") {
    REQUIRE_FALSE(TextFixEngine::check_word("a").has_value());
    REQUIRE_FALSE(TextFixEngine::check_word("is").has_value());
}
