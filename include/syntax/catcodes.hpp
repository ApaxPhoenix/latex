#pragma once

#include <array>
#include <vector>
#include <cstdint> // MSYS requires this, since it's not provided by default.

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

        struct Entry {
            std::size_t symbol;
            Category category;
        };

        constexpr CatCodes() noexcept {
            this->table.fill(Category::Other);

            for (int code = 'a'; code <= 'z'; ++code) {
                this->table[static_cast<std::size_t>(code)] = Category::Letter;
            }
            for (int code = 'A'; code <= 'Z'; ++code) {
                this->table[static_cast<std::size_t>(code)] = Category::Letter;
            }

            this->table[static_cast<std::size_t>('\\')] = Category::Escape;
            this->table[static_cast<std::size_t>('{')]  = Category::Group;
            this->table[static_cast<std::size_t>('}')]  = Category::Group;
            this->table[static_cast<std::size_t>('$')]  = Category::Shift;
            this->table[static_cast<std::size_t>('&')]  = Category::Align;
            this->table[static_cast<std::size_t>('#')]  = Category::Parameter;
            this->table[static_cast<std::size_t>('^')]  = Category::Mark;
            this->table[static_cast<std::size_t>('_')]  = Category::Index;
            this->table[static_cast<std::size_t>('%')]  = Category::Comment;
            this->table[static_cast<std::size_t>(' ')]  = Category::Space;
            this->table[static_cast<std::size_t>('\t')] = Category::Space;
            this->table[static_cast<std::size_t>('\n')] = Category::Space;
            this->table[static_cast<std::size_t>('~')]  = Category::Active;
        }

        void set(const char symbol, const Category category, const bool global = false) {
            const auto code = static_cast<std::size_t>(static_cast<unsigned char>(symbol));
            if (code >= this->table.size()) return;

            if (!global && !this->marks.empty()) {
                this->history.push_back(Entry{code, this->table[code]});
            }

            this->table[code] = category;
        }

        [[nodiscard]] constexpr Category get(const char symbol) const noexcept {
            return this->table[static_cast<std::size_t>(static_cast<unsigned char>(symbol))];
        }

        void push() {
            this->marks.push_back(this->history.size());
        }

        void pop() {
            if (this->marks.empty()) return;

            const std::size_t mark = this->marks.back();
            this->marks.pop_back();

            for (std::size_t count = this->history.size() - mark; count > 0; --count) {
                const auto [symbol, category] = this->history.back();
                this->history.pop_back();
                this->table[symbol] = category;
            }
        }

    private:
        std::array<Category, 256> table{};
        std::vector<Entry> history{};
        std::vector<std::size_t> marks{};
    };

}