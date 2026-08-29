#pragma once

#include <array>
#include <cstddef>

namespace syntax::primitives::casecodes {

    class Cases {
    public:
        constexpr Cases() noexcept {
            for (int character = 0; character < 256; ++character) {
                uppers[static_cast<std::size_t>(character)] = static_cast<char>(character);
                lowers[static_cast<std::size_t>(character)] = static_cast<char>(character);
            }
            for (int character = 'a'; character <= 'z'; ++character) {
                uppers[static_cast<std::size_t>(character)] = static_cast<char>(character - ('a' - 'A'));
            }
            for (int character = 'A'; character <= 'Z'; ++character) {
                lowers[static_cast<std::size_t>(character)] = static_cast<char>(character + ('a' - 'A'));
            }
        }

        constexpr void upper(const char character, const char replacement) noexcept {
            uppers[static_cast<std::size_t>(static_cast<unsigned char>(character))] = replacement;
        }

        constexpr void lower(const char character, const char replacement) noexcept {
            lowers[static_cast<std::size_t>(static_cast<unsigned char>(character))] = replacement;
        }

        [[nodiscard]] constexpr char upper(const char character) const noexcept {
            return uppers[static_cast<std::size_t>(static_cast<unsigned char>(character))];
        }

        [[nodiscard]] constexpr char lower(const char character) const noexcept {
            return lowers[static_cast<std::size_t>(static_cast<unsigned char>(character))];
        }

    private:
        std::array<char, 256> uppers{};
        std::array<char, 256> lowers{};
    };

}