#pragma once

#include <array>
#include <vector>

namespace syntax {

    class CatCodes {
    public:
        enum class Category : std::uint8_t {
            Escape,    // Command prefix (default: \)
            Group,     // Scope delimiters (default: { and })
            Shift,     // Math mode toggle (default: $)
            Align,     // Alignment separator (default: &)
            Parameter, // Macro argument marker (default: #)
            Mark,      // Superscript marker (default: ^)
            Index,     // Subscript marker (default: _)
            Letter,    // Alphabetic identifier characters (a-z, A-Z)
            Other,     // Numbers, punctuation, and unmapped text
            Space,     // Blank space, tab, or newline
            Comment,   // Rest-of-line comment (default: %)
            Active,    // Character acting directly as a command (default: ~)
            Ignore,    // Character completely skipped by the lexer
            Invalid    // Unprintable ASCII or illegal byte
        };

        constexpr CatCodes() noexcept {
            entries.fill(Category::Other);

            for (int code = 'a'; code <= 'z'; ++code) {
                entries[static_cast<std::size_t>(code)] = Category::Letter;
            }
            for (int code = 'A'; code <= 'Z'; ++code) {
                entries[static_cast<std::size_t>(code)] = Category::Letter;
            }

            entries[static_cast<std::size_t>('\\')] = Category::Escape;
            entries[static_cast<std::size_t>('{')]  = Category::Group;
            entries[static_cast<std::size_t>('}')]  = Category::Group;
            entries[static_cast<std::size_t>('$')]  = Category::Shift;
            entries[static_cast<std::size_t>('&')]  = Category::Align;
            entries[static_cast<std::size_t>('#')]  = Category::Parameter;
            entries[static_cast<std::size_t>('^')]  = Category::Mark;
            entries[static_cast<std::size_t>('_')]  = Category::Index;
            entries[static_cast<std::size_t>('%')]  = Category::Comment;
            entries[static_cast<std::size_t>(' ')]  = Category::Space;
            entries[static_cast<std::size_t>('\t')] = Category::Space;
            entries[static_cast<std::size_t>('\n')] = Category::Space;
            entries[static_cast<std::size_t>('~')]  = Category::Active;
        }

        constexpr void set(const char symbol, const Category category) noexcept {
            entries[static_cast<std::size_t>(static_cast<unsigned char>(symbol))] = category;
        }

        [[nodiscard]] constexpr Category get(const char symbol) const noexcept {
            return entries[static_cast<std::size_t>(static_cast<unsigned char>(symbol))];
        }

        void push() {
            stacks.push_back(entries);
        }

        void pop() {
            if (!stacks.empty()) {
                entries = stacks.back();
                stacks.pop_back();
            }
        }

    private:
        std::array<Category, 256> entries{};
        std::vector<std::array<Category, 256>> stacks{};
    };

}