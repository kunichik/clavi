#include <catch2/catch_test_macros.hpp>
#include "clavi/detector.hpp"

#include <filesystem>
#include <fstream>
#include <vector>
#include <cmath>

#define XXH_INLINE_ALL
#include <xxhash.h>

namespace {

namespace fs = std::filesystem;

// Write a minimal keyboard_map.bin
void write_kmap(const fs::path& path,
                const std::vector<std::pair<uint32_t, uint32_t>>& pairs) {
    std::vector<std::pair<uint32_t, uint32_t>> sorted = pairs;
    std::sort(sorted.begin(), sorted.end());
    std::ofstream f(path, std::ios::binary);
    const uint8_t magic[4] = {'K', 'M', 'A', 'P'};
    f.write(reinterpret_cast<const char*>(magic), 4);
    const uint32_t count = static_cast<uint32_t>(sorted.size());
    f.write(reinterpret_cast<const char*>(&count), 4);
    for (const auto& [src, tgt] : sorted) {
        f.write(reinterpret_cast<const char*>(&src), 4);
        f.write(reinterpret_cast<const char*>(&tgt), 4);
    }
}

// Write a dictionary.bin
void write_dict(const fs::path& path, const std::vector<std::string>& words) {
    const std::size_t count = words.size();
    std::size_t table_size = static_cast<std::size_t>(std::ceil(static_cast<double>(count) / 0.7));
    std::size_t p2 = 1;
    while (p2 < table_size) p2 <<= 1;
    table_size = std::max(p2, std::size_t{4});

    std::vector<uint64_t> table(table_size, 0ULL);
    for (const auto& word : words) {
        std::string lower;
        for (unsigned char c : word) lower += static_cast<char>(std::tolower(c));
        uint64_t h = XXH3_64bits(lower.data(), lower.size());
        if (h == 0) h = 1;
        std::size_t idx = static_cast<std::size_t>(h) & (table_size - 1);
        while (table[idx] != 0) idx = (idx + 1) & (table_size - 1);
        table[idx] = h;
    }

    std::ofstream f(path, std::ios::binary);
    const uint8_t magic[4] = {'C', 'L', 'A', 'V'};
    f.write(reinterpret_cast<const char*>(magic), 4);
    const uint32_t cnt = static_cast<uint32_t>(count);
    f.write(reinterpret_cast<const char*>(&cnt), 4);
    f.write(reinterpret_cast<const char*>(table.data()),
            static_cast<std::streamsize>(table_size * 8));
}

// Write a pack.toml
void write_pack_toml(const fs::path& path, std::string_view locale) {
    std::ofstream f(path);
    f << "[pack]\n";
    f << "locale = \"" << locale << "\"\n";
    f << "name = \"Test Pack\"\n";
    f << "version = \"1.0.0\"\n";
    f << "[features]\n";
    f << "switch = true\n";
    f << "[files]\n";
    f << "keyboard_map = \"keyboard_map.bin\"\n";
    f << "dictionary = \"dictionary.bin\"\n";
}

// Ukrainian QWERTY → EN key mapping (partial, enough for "ghbdtn" → "привіт")
// g->п h->р b->и d->в f->а i->и t->е n->т
// Actually ghbdtn typed on EN layout should map to привіт on UK layout
// EN key -> UK character mapping (what you get if you press EN keys on UK layout):
// g -> п (U+043F)
// h -> р (U+0440)
// b -> и (U+0438)  (note: b on EN keyboard = и on Ukrainian)
// d -> в (U+0432)
// t -> е (U+0435)
// n -> т (U+0442)
// i -> и (U+0438)  

// Actually let me think about this more carefully.
// Ukrainian QWERTY layout (standard):
// q->й w->ц e->у r->к t->е y->н u->г i->ш o->щ p->з
// a->ф s->і d->в f->а g->п h->р j->о k->л l->д ;->ж
// z->я x->ч c->с v->м b->и n->т m->ь

// So "ghbdtn" on EN keyboard = г(g)р(h)и(b)в(d)е(t)т(n) = "привіт"? 
// Wait, that spells г-р-и-в-е-т = "гривет" not "привіт"
// Actually the transliteration ghbdtn is the standard Russian transliteration joke
// But for Ukrainian: п=g, р=h, и=b, в=d, і=s, т=n
// "привіт" = п-р-и-в-і-т
// In EN keyboard positions: g-h-b-d-s-n
// So "ghbdsn" → "привіт"? Hmm, let me just use "ghbdtn" → something in Ukrainian
// The test in the spec says: analyzer.analyze("ghbdtn") should return SwitchAndRetype with "привіт"
// So let me figure out the exact mapping.
// Ukrainian QWERTY: п=g, р=h, и=b, в=d, і=s (not i!), т=n
// привіт = п(g) р(h) и(b) в(d) і(s) т(n) = "ghbdsn"
// But the spec uses "ghbdtn"... Let me re-read
// The spec says: auto result = detector.analyze("ghbdtn"); assert corrected_text == "привіт"
// Let me check Ukrainian keyboard layout more carefully
// Standard Ukrainian keyboard (101/102-key):
// Row 3 (QWERTY): й ц у к е н г ш щ з х ї
// Row 2 (ASDF):   ф і в а п р о л д ж є ї
// Row 1 (ZXCV):   я ч с м и т ь б ю .
// So: q=й w=ц e=у r=к t=е y=н g=п h=р b=и d=в (wait)
// Actually more precisely:
// q->й, w->ц, e->у, r->к, t->е, y->н, u->г, i->ш, o->щ, p->з
// a->ф, s->і, d->в, f->а, g->п, h->р, j->о, k->л, l->д
// z->я, x->ч, c->с, v->м, b->и, n->т, m->ь
// So привіт = п р и в і т
// п=g, р=h, и=b, в=d, і=s, т=n
// ghbdsn → привіт (not ghbdtn!)
// 
// But the spec explicitly says "ghbdtn". Let me check again...
// Maybe it's a different common layout. Let me just look at the comment - it says
// typed on EN layout, result is привіт. So the test just defines what the mapping should be.
// For my test purposes, I'll use the correct Ukrainian QWERTY mapping where ghbdsn → привіт.
// The spec example might have a typo (using t instead of s for 'і'), or use a different layout.
// Since I control the test pack, I'll use ghbdsn → привіт to match the actual UK QWERTY layout.
// Actually wait - looking at it again - ghbdtn: 
// g=п, h=р, b=и, d=в, t=е, n=т → п-р-и-в-е-т = "привет" (Russian word!)
// For Ukrainian "привіт": п-р-и-в-і-т where і≠е
// So "ghbdtn" would give "привет" (if е) not "привіт"
// The spec might just be using it as a conceptual example
// Let me use "ghbdsn" → "привіт" in my test which is more correct for Ukrainian QWERTY

const std::vector<std::pair<uint32_t, uint32_t>> UK_KMAP = {
    // EN key codepoint -> UK codepoint (what Ukrainian you get pressing those EN keys)
    {0x62, 0x0438},  // b -> и
    {0x63, 0x0441},  // c -> с
    {0x64, 0x0432},  // d -> в
    {0x65, 0x0443},  // e -> у
    {0x66, 0x0430},  // f -> а
    {0x67, 0x043F},  // g -> п
    {0x68, 0x0440},  // h -> р
    {0x69, 0x0448},  // i -> ш
    {0x6A, 0x043E},  // j -> о
    {0x6B, 0x043B},  // k -> л
    {0x6C, 0x0434},  // l -> д
    {0x6D, 0x044C},  // m -> ь
    {0x6E, 0x0442},  // n -> т
    {0x6F, 0x0449},  // o -> щ
    {0x70, 0x0437},  // p -> з
    {0x71, 0x0439},  // q -> й
    {0x72, 0x043A},  // r -> к
    {0x73, 0x0456},  // s -> і
    {0x74, 0x0435},  // t -> е
    {0x75, 0x0433},  // u -> г
    {0x76, 0x043C},  // v -> м
    {0x77, 0x0446},  // w -> ц
    {0x78, 0x0447},  // x -> ч
    {0x79, 0x043D},  // y -> н
    {0x7A, 0x044F},  // z -> я
};

// "ghbdsn" -> г(u)... wait no. g->п, h->р, b->и, d->в, s->і, n->т = "привіт"
// So the typed sequence is "ghbdsn" for "привіт"

fs::path setup_test_packs() {
    const auto tmp = fs::temp_directory_path() / "clavi_test_packs";
    fs::create_directories(tmp / "uk");
    fs::create_directories(tmp / "en");

    // UK pack
    write_pack_toml(tmp / "uk" / "pack.toml", "uk");
    write_dict(tmp / "uk" / "dictionary.bin", {"привіт", "клавіатура", "мова", "слово"});
    write_kmap(tmp / "uk" / "keyboard_map.bin", UK_KMAP);

    // EN pack: map UK chars -> EN chars (reverse of UK_KMAP)
    std::vector<std::pair<uint32_t, uint32_t>> en_kmap;
    for (const auto& [en, uk] : UK_KMAP) {
        en_kmap.push_back({uk, en});
    }
    write_pack_toml(tmp / "en" / "pack.toml", "en");
    write_dict(tmp / "en" / "dictionary.bin", {"hello", "world", "keyboard", "language"});
    write_kmap(tmp / "en" / "keyboard_map.bin", en_kmap);

    return tmp;
}

} // namespace

TEST_CASE("Detector: ghbdsn switches to Ukrainian (pryvit)", "[detector]") {
    const auto packs_dir = setup_test_packs();
    clavi::Detector det;
    REQUIRE(det.load_pack((packs_dir / "uk").string()));
    REQUIRE(det.load_pack((packs_dir / "en").string()));

    // "ghbdsn" typed on EN keyboard = "привіт" in Ukrainian
    const auto result = det.analyze("ghbdsn");
    REQUIRE(result.action == clavi::Action::SwitchAndRetype);
    REQUIRE(result.target_locale == "uk");
    REQUIRE(result.corrected_text == "привіт");
}

TEST_CASE("Detector: 'hello' is NoAction", "[detector]") {
    const auto packs_dir = setup_test_packs();
    clavi::Detector det;
    REQUIRE(det.load_pack((packs_dir / "uk").string()));
    REQUIRE(det.load_pack((packs_dir / "en").string()));

    const auto result = det.analyze("hello");
    REQUIRE(result.action == clavi::Action::NoAction);
}

TEST_CASE("Detector: Ukrainian text typed correctly is NoAction", "[detector]") {
    const auto packs_dir = setup_test_packs();
    clavi::Detector det;
    REQUIRE(det.load_pack((packs_dir / "uk").string()));
    REQUIRE(det.load_pack((packs_dir / "en").string()));

    const auto result = det.analyze("привіт");
    REQUIRE(result.action == clavi::Action::NoAction);
}

TEST_CASE("Detector: short word skip (<=2 codepoints) is always NoAction", "[detector]") {
    const auto packs_dir = setup_test_packs();
    clavi::Detector det;
    REQUIRE(det.load_pack((packs_dir / "uk").string()));
    REQUIRE(det.load_pack((packs_dir / "en").string()));

    REQUIRE(det.analyze("").action == clavi::Action::NoAction);
    REQUIRE(det.analyze("a").action == clavi::Action::NoAction);
    REQUIRE(det.analyze("ab").action == clavi::Action::NoAction);
}

TEST_CASE("Detector: ambiguous token skip-list is NoAction", "[detector]") {
    const auto packs_dir = setup_test_packs();
    clavi::Detector det;
    REQUIRE(det.load_pack((packs_dir / "uk").string()));
    REQUIRE(det.load_pack((packs_dir / "en").string()));

    // These are in the hardcoded skip list
    REQUIRE(det.analyze("a").action == clavi::Action::NoAction);
    REQUIRE(det.analyze("i").action == clavi::Action::NoAction);
    REQUIRE(det.analyze("ok").action == clavi::Action::NoAction);
}

TEST_CASE("Detector: user-defined skip_words are excluded", "[detector]") {
    const auto packs_dir = setup_test_packs();
    clavi::Detector det;
    REQUIRE(det.load_pack((packs_dir / "uk").string()));
    REQUIRE(det.load_pack((packs_dir / "en").string()));

    // "ghbdsn" would normally trigger SwitchAndRetype (maps to привіт)
    const auto before = det.analyze("ghbdsn");
    REQUIRE(before.action == clavi::Action::SwitchAndRetype);

    // After adding it to skip_words, it should be NoAction
    det.set_skip_words({"ghbdsn"});
    const auto after = det.analyze("ghbdsn");
    REQUIRE(after.action == clavi::Action::NoAction);
}

TEST_CASE("Detector: unknown garbage word is NoAction", "[detector]") {
    const auto packs_dir = setup_test_packs();
    clavi::Detector det;
    REQUIRE(det.load_pack((packs_dir / "uk").string()));
    REQUIRE(det.load_pack((packs_dir / "en").string()));

    REQUIRE(det.analyze("xkcd9999zzzz").action == clavi::Action::NoAction);
}

TEST_CASE("Detector: rejects 'ru' pack", "[detector]") {
    const auto tmp = fs::temp_directory_path() / "clavi_test_packs_ru";
    fs::create_directories(tmp / "ru");
    write_pack_toml(tmp / "ru" / "pack.toml", "ru");

    clavi::Detector det;
    REQUIRE_FALSE(det.load_pack((tmp / "ru").string()));
    REQUIRE(det.pack_count() == 0);
}
