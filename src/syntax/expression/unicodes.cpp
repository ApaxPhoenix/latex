#include "syntax/expression/unicodes.hpp"
#include "lookup.hpp"

#include <array>
#include <cstring>

namespace syntax::expression {

    const std::uint32_t Unicodes::invalid = 0xFFFD;

    Unicodes::Unicodes() = default;

    void Unicodes::compose(const std::string_view name, const std::uint32_t codepoint, const Category category) {
        overrides.insert_or_assign(std::string(name), Symbol{codepoint, category});
    }

    void Unicodes::dispose(const std::string_view name) {
        overrides.erase(std::string(name));
    }

    std::optional<Unicodes::Symbol> Unicodes::query(const std::string_view name) const noexcept {
        if (name.empty()) return std::nullopt;

        if (const auto iterator = overrides.find(name); iterator != overrides.end()) {
            return iterator->second;
        }

        char stack[128];
        const char* text = nullptr;

        if (name.size() < sizeof(stack)) {
            std::memcpy(stack, name.data(), name.size());
            stack[name.size()] = '\0';
            text = stack;
        } else {
            const std::string heap(name);
            if (const auto* entry = Lookup::query(heap.c_str(), static_cast<unsigned int>(heap.size()))) {
                return Symbol{entry->codepoint, static_cast<Category>(entry->category)};
            }
            return std::nullopt;
        }

        if (const auto* entry = Lookup::query(text, static_cast<unsigned int>(name.size()))) {
            return Symbol{entry->codepoint, static_cast<Category>(entry->category)};
        }

        return std::nullopt;
    }

}