#include <catch2/catch_test_macros.hpp>
#include "clavi/undo_stack.hpp"

TEST_CASE("UndoStack: starts empty", "[undo_stack]") {
    clavi::UndoStack stack;
    REQUIRE(stack.empty());
    REQUIRE(stack.size() == 0);
}

TEST_CASE("UndoStack: push and pop single entry", "[undo_stack]") {
    clavi::UndoStack stack;
    stack.push({"ghbdsn", "привіт", "en", "uk"});
    REQUIRE_FALSE(stack.empty());
    REQUIRE(stack.size() == 1);

    const auto entry = stack.pop();
    REQUIRE(entry.has_value());
    REQUIRE(entry->original_text == "ghbdsn");
    REQUIRE(entry->switched_text == "привіт");
    REQUIRE(entry->locale_before == "en");
    REQUIRE(entry->locale_after == "uk");
    REQUIRE(stack.empty());
}

TEST_CASE("UndoStack: pop from empty returns nullopt", "[undo_stack]") {
    clavi::UndoStack stack;
    REQUIRE_FALSE(stack.pop().has_value());
}

TEST_CASE("UndoStack: LIFO order", "[undo_stack]") {
    clavi::UndoStack stack;
    stack.push({"first", "FIRST", "en", "uk"});
    stack.push({"second", "SECOND", "en", "uk"});
    stack.push({"third", "THIRD", "en", "uk"});

    REQUIRE(stack.pop()->original_text == "third");
    REQUIRE(stack.pop()->original_text == "second");
    REQUIRE(stack.pop()->original_text == "first");
    REQUIRE_FALSE(stack.pop().has_value());
}

TEST_CASE("UndoStack: capacity_10 -- 11th push evicts oldest", "[undo_stack]") {
    clavi::UndoStack stack;
    REQUIRE(clavi::UndoStack::CAPACITY == 10);

    for (std::size_t i = 0; i < 10; ++i) {
        stack.push({std::to_string(i), "", "en", "uk"});
    }
    REQUIRE(stack.size() == 10);

    // 11th push — oldest (0) should be evicted
    stack.push({"10", "", "en", "uk"});
    REQUIRE(stack.size() == 10);

    // Pop all 10, most recent first
    std::vector<std::string> popped;
    while (!stack.empty()) {
        popped.push_back(stack.pop()->original_text);
    }
    // Should have 1 through 10 (not 0)
    REQUIRE(popped.size() == 10);
    REQUIRE(popped[0] == "10");
    REQUIRE(popped[9] == "1");
}

TEST_CASE("UndoStack: push beyond capacity many times", "[undo_stack]") {
    clavi::UndoStack stack;
    for (int i = 0; i < 50; ++i) {
        stack.push({std::to_string(i), "", "en", "uk"});
    }
    REQUIRE(stack.size() == clavi::UndoStack::CAPACITY);

    // Most recent 10 entries should be 40..49
    const auto top = stack.pop();
    REQUIRE(top.has_value());
    REQUIRE(top->original_text == "49");
}
