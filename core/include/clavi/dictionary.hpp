#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace clavi {

class Dictionary {
public:
    Dictionary() = default;

    [[nodiscard]] bool load(std::string_view path);

    [[nodiscard]] bool contains(std::string_view word) const noexcept;

    [[nodiscard]] bool empty() const noexcept { return table_.empty(); }

    [[nodiscard]] std::size_t capacity() const noexcept { return table_.size(); }

private:
    std::vector<uint64_t> table_;

    [[nodiscard]] static uint64_t hash_word(std::string_view word) noexcept;
    [[nodiscard]] static uint32_t read_u32_le(const uint8_t* p) noexcept;
    [[nodiscard]] static uint64_t read_u64_le(const uint8_t* p) noexcept;
};

} // namespace clavi
