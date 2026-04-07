#include "clavi/undo_stack.hpp"

namespace clavi {

void UndoStack::push(UndoEntry entry) noexcept {
    entries_[head_] = std::move(entry);
    head_ = (head_ + 1) % CAPACITY;
    if (size_ < CAPACITY) ++size_;
}

std::optional<UndoEntry> UndoStack::pop() noexcept {
    if (size_ == 0) return std::nullopt;
    --size_;
    head_ = (head_ + CAPACITY - 1) % CAPACITY;
    return std::move(entries_[head_]);
}

} // namespace clavi
