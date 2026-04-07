#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>

namespace clavi {

struct UndoEntry {
    std::string original_text;
    std::string switched_text;
    std::string locale_before;
    std::string locale_after;
};

class UndoStack {
public:
    static constexpr std::size_t CAPACITY = 10;

    UndoStack() = default;

    void push(UndoEntry entry) noexcept;

    [[nodiscard]] std::optional<UndoEntry> pop() noexcept;

    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }

private:
    std::array<UndoEntry, CAPACITY> entries_{};
    std::size_t head_{0};
    std::size_t size_{0};
};

} // namespace clavi
