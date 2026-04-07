#include <catch2/catch_test_macros.hpp>
#include "clavi/word_buffer.hpp"

#include <string>
#include <vector>

// ── Helpers ───────────────────────────────────────────────────────────────────

static void feed_ascii(clavi::WordBuffer& buf, const char* s) {
    for (; *s; ++s)
        buf.feed_codepoint(static_cast<uint32_t>(static_cast<unsigned char>(*s)));
}

// Feed a UTF-8 string codepoint by codepoint
static void feed_utf8(clavi::WordBuffer& buf, const std::string& s) {
    const char* p = s.data();
    const char* end = p + s.size();
    while (p < end) {
        const auto b0 = static_cast<uint8_t>(*p++);
        uint32_t cp = b0;
        int extra = 0;
        if (b0 >= 0xF0) { extra = 3; cp = b0 & 0x07; }
        else if (b0 >= 0xE0) { extra = 2; cp = b0 & 0x0F; }
        else if (b0 >= 0xC0) { extra = 1; cp = b0 & 0x1F; }
        for (int i = 0; i < extra && p < end; ++i) {
            cp = (cp << 6) | (static_cast<uint8_t>(*p++) & 0x3F);
        }
        buf.feed_codepoint(cp);
    }
}

// ── Tests ─────────────────────────────────────────────────────────────────────

TEST_CASE("WordBuffer: empty initially", "[word_buffer]") {
    clavi::WordBuffer buf;
    REQUIRE(buf.empty());
    REQUIRE(buf.current().empty());
}

TEST_CASE("WordBuffer: accumulates ASCII chars", "[word_buffer]") {
    clavi::WordBuffer buf;
    feed_ascii(buf, "hell");
    REQUIRE(buf.current() == "hell");
    REQUIRE(!buf.empty());
}

TEST_CASE("WordBuffer: space triggers commit", "[word_buffer]") {
    std::vector<std::string> commits;
    clavi::WordBuffer buf;
    buf.set_commit_callback([&](const std::string& w) { commits.push_back(w); });

    feed_ascii(buf, "hello");
    buf.feed_codepoint(' ');

    REQUIRE(commits.size() == 1);
    REQUIRE(commits[0] == "hello");
    REQUIRE(buf.empty());
}

TEST_CASE("WordBuffer: multiple words on spaces", "[word_buffer]") {
    std::vector<std::string> commits;
    clavi::WordBuffer buf;
    buf.set_commit_callback([&](const std::string& w) { commits.push_back(w); });

    feed_ascii(buf, "one");
    buf.feed_codepoint(' ');
    feed_ascii(buf, "two");
    buf.feed_codepoint(' ');

    REQUIRE(commits.size() == 2);
    REQUIRE(commits[0] == "one");
    REQUIRE(commits[1] == "two");
}

TEST_CASE("WordBuffer: no commit on empty flush", "[word_buffer]") {
    std::vector<std::string> commits;
    clavi::WordBuffer buf;
    buf.set_commit_callback([&](const std::string& w) { commits.push_back(w); });

    buf.feed_codepoint(' '); // boundary on empty buf
    REQUIRE(commits.empty());
}

TEST_CASE("WordBuffer: backspace erases last ASCII char", "[word_buffer]") {
    clavi::WordBuffer buf;
    feed_ascii(buf, "hell");
    buf.feed_backspace();
    REQUIRE(buf.current() == "hel");
    buf.feed_backspace();
    buf.feed_backspace();
    buf.feed_backspace();
    REQUIRE(buf.empty());
}

TEST_CASE("WordBuffer: backspace on empty is no-op", "[word_buffer]") {
    clavi::WordBuffer buf;
    buf.feed_backspace(); // must not crash
    REQUIRE(buf.empty());
}

TEST_CASE("WordBuffer: feed_clear empties buffer without commit", "[word_buffer]") {
    std::vector<std::string> commits;
    clavi::WordBuffer buf;
    buf.set_commit_callback([&](const std::string& w) { commits.push_back(w); });

    feed_ascii(buf, "hello");
    buf.feed_clear();
    REQUIRE(buf.empty());
    REQUIRE(commits.empty());
}

TEST_CASE("WordBuffer: flush emits current word", "[word_buffer]") {
    std::vector<std::string> commits;
    clavi::WordBuffer buf;
    buf.set_commit_callback([&](const std::string& w) { commits.push_back(w); });

    feed_ascii(buf, "world");
    buf.flush();

    REQUIRE(commits.size() == 1);
    REQUIRE(commits[0] == "world");
    REQUIRE(buf.empty());
}

TEST_CASE("WordBuffer: punctuation is a word boundary", "[word_buffer]") {
    std::vector<std::string> commits;
    clavi::WordBuffer buf;
    buf.set_commit_callback([&](const std::string& w) { commits.push_back(w); });

    // period
    feed_ascii(buf, "end");
    buf.feed_codepoint('.');
    REQUIRE(commits.size() == 1);
    REQUIRE(commits[0] == "end");

    // comma
    feed_ascii(buf, "next");
    buf.feed_codepoint(',');
    REQUIRE(commits.size() == 2);
    REQUIRE(commits[1] == "next");
}

TEST_CASE("WordBuffer: Ukrainian codepoints accumulated correctly", "[word_buffer]") {
    std::vector<std::string> commits;
    clavi::WordBuffer buf;
    buf.set_commit_callback([&](const std::string& w) { commits.push_back(w); });

    // "привіт" in UTF-8
    const std::string pryvit = "\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82";
    feed_utf8(buf, pryvit);
    buf.flush();

    REQUIRE(commits.size() == 1);
    REQUIRE(commits[0] == pryvit);
}

TEST_CASE("WordBuffer: backspace removes last UTF-8 codepoint", "[word_buffer]") {
    clavi::WordBuffer buf;
    // "ти" — 2 Cyrillic chars, each 2 bytes
    const std::string ty = "\xD1\x82\xD0\xB8"; // т + и
    feed_utf8(buf, ty);
    REQUIRE(buf.current() == ty); // 4 bytes

    buf.feed_backspace(); // removes и (2 bytes)
    REQUIRE(buf.current().size() == 2); // only т left
    REQUIRE(buf.current() == "\xD1\x82");

    buf.feed_backspace(); // removes т
    REQUIRE(buf.empty());
}

TEST_CASE("WordBuffer: overflow protection clears buffer", "[word_buffer]") {
    std::vector<std::string> commits;
    clavi::WordBuffer buf(10); // tiny max
    buf.set_commit_callback([&](const std::string& w) { commits.push_back(w); });

    // Fill past max_word_bytes
    for (int i = 0; i < 20; ++i) buf.feed_codepoint('a');

    // Buffer should have been cleared (no crash, no commit yet)
    REQUIRE(commits.empty());
}
