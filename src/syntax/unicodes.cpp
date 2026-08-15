#include "syntax/unicodes.hpp"
#include "lookup.hpp"

namespace syntax {

    const std::uint32_t Unicodes::invalid = 0xFFFD;

    Unicodes::Unicodes() = default;

    void Unicodes::compose(const std::string_view name, const std::uint32_t codepoint, const Type type) {
        overrides.insert_or_assign(std::string(name), Symbol{codepoint, type});
    }

    void Unicodes::dispose(const std::string_view name) {
        overrides.erase(name);
    }

    std::optional<Unicodes::Symbol> Unicodes::query(const std::string_view name) const noexcept {
        if (const auto iterator = overrides.find(name); iterator != overrides.end()) {
            return iterator->second;
        }

        if (const auto* entry = Lookup::query(name.data(), static_cast<unsigned int>(name.size()))) {
            return Symbol{entry->codepoint, entry->type};
        }

        return std::nullopt;
    }

    std::uint32_t Unicodes::next(std::string_view& stream) noexcept {
        if (stream.empty()) {
            return 0;
        }

        const auto lead = static_cast<std::uint8_t>(stream[0]);
        std::size_t length = 1;
        std::uint32_t codepoint = 0;

        if (lead < 0x80) {
            codepoint = lead;
        } else if (lead >> 5 == 0x06) {
            length = 2;
            codepoint = lead & 0x1F;
        } else if (lead >> 4 == 0x0E) {
            length = 3;
            codepoint = lead & 0x0F;
        } else if (lead >> 3 == 0x1E) {
            length = 4;
            codepoint = lead & 0x07;
        } else {
            stream.remove_prefix(1);
            return invalid;
        }

        if (stream.size() < length) {
            stream = {};
            return invalid;
        }

        for (std::size_t index = 1; index < length; ++index) {
            const auto byte = static_cast<std::uint8_t>(stream[index]);
            if (byte >> 6 != 0x02) {
                stream.remove_prefix(index);
                return invalid;
            }
            codepoint = codepoint << 6 | byte & 0x3F;
        }

        stream.remove_prefix(length);
        return codepoint;
    }

}