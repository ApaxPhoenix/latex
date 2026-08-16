#pragma once

#include <array>
#include <vector>

namespace syntax {

    class CatCodes {
    public:
        enum class Type : std::uint8_t {
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
            entries.fill(Type::Other);

            for (int code = 'a'; code <= 'z'; ++code) {
                entries[static_cast<std::size_t>(code)] = Type::Letter;
            }
            for (int code = 'A'; code <= 'Z'; ++code) {
                entries[static_cast<std::size_t>(code)] = Type::Letter;
            }

            entries[static_cast<std::size_t>('\\')] = Type::Escape;
            entries[static_cast<std::size_t>('{')]  = Type::Group;
            entries[static_cast<std::size_t>('}')]  = Type::Group;
            entries[static_cast<std::size_t>('$')]  = Type::Shift;
            entries[static_cast<std::size_t>('&')]  = Type::Align;
            entries[static_cast<std::size_t>('#')]  = Type::Parameter;
            entries[static_cast<std::size_t>('^')]  = Type::Mark;
            entries[static_cast<std::size_t>('_')]  = Type::Index;
            entries[static_cast<std::size_t>('%')]  = Type::Comment;
            entries[static_cast<std::size_t>(' ')]  = Type::Space;
            entries[static_cast<std::size_t>('\t')] = Type::Space;
            entries[static_cast<std::size_t>('\n')] = Type::Space;
            entries[static_cast<std::size_t>('~')]  = Type::Active;
        }

        constexpr void set(const char symbol, const Type category) noexcept {
            entries[static_cast<std::size_t>(static_cast<unsigned char>(symbol))] = category;
        }

        [[nodiscard]] constexpr Type get(const char symbol) const noexcept {
            return entries[static_cast<std::size_t>(static_cast<unsigned char>(symbol))];
        }

        void push() {
            stack.push_back(entries);
        }

        void pop() {
            if (!stack.empty()) {
                entries = stack.back();
                stack.pop_back();
            }
        }

    private:
        std::array<Type, 256> entries{};
        std::vector<std::array<Type, 256>> stack{};
    };

}