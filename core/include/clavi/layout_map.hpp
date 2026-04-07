#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace clavi {

struct KeyPair {
    uint32_t source;
    uint32_t target;
};

class LayoutMap {
public:
    LayoutMap() = default;

    [[nodiscard]] bool load(std::string_view path);

    [[nodiscard]] std::optional<uint32_t> remap_codepoint(uint32_t cp) const noexcept;

    [[nodiscard]] std::string remap(std::string_view text) const;

    [[nodiscard]] bool empty() const noexcept { return pairs_.empty(); }

    [[nodiscard]] std::size_t size() const noexcept { return pairs_.size(); }

private:
    std::vector<KeyPair> pairs_;

    [[nodiscard]] static uint32_t read_u32_le(const uint8_t* p) noexcept;
};

} // namespace clavi
