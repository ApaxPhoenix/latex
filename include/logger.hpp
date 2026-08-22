#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <fstream>
#include <mutex>
#include <format>
#include <source_location>
#include <utility>

class Logger {
public:
    enum class Type : std::uint32_t {
        None      = 0,
        Lexer     = 1 << 0,
        Mouth     = 1 << 1,
        Parser    = 1 << 2,
        Layout    = 1 << 3,
        Memory    = 1 << 4,
        Semantics = 1 << 5,
        All       = 0xFFFFFFFF
    };

    friend constexpr Type operator|(const Type alpha, const Type beta) noexcept {
        return static_cast<Type>(static_cast<std::uint32_t>(alpha) | static_cast<std::uint32_t>(beta));
    }

    friend constexpr Type operator&(const Type alpha, const Type beta) noexcept {
        return static_cast<Type>(static_cast<std::uint32_t>(alpha) & static_cast<std::uint32_t>(beta));
    }

    friend constexpr Type operator~(const Type alpha) noexcept {
        return static_cast<Type>(~static_cast<std::uint32_t>(alpha));
    }

    friend constexpr Type& operator|=(Type& alpha, const Type beta) noexcept {
        alpha = alpha | beta;
        return alpha;
    }

    friend constexpr Type& operator&=(Type& alpha, const Type beta) noexcept {
        alpha = alpha & beta;
        return alpha;
    }

    enum class Level : std::uint8_t {
        Traceback,
        Debug,
        Informative,
        Warning,
        Error
    };

    static void init(int count, char** arguments);
    static void types(Type target) noexcept;
    static void enable(Type target) noexcept;
    static void disable(Type target) noexcept;
    static void level(Level value) noexcept;
    static void color(bool flag) noexcept;
    static void file(const std::string& path);
    static void close();

    [[nodiscard]] static bool check(Type target, Level value = Level::Debug) noexcept;

    static void log(Type target, Level value, std::string_view text,
                    const std::source_location& location = std::source_location::current());

    static void log(const Type target, const std::string_view text,
                    const std::source_location& location = std::source_location::current()) {
        log(target, Level::Debug, text, location);
    }

    template <typename... Arguments>
    static void fmt(const Type target, const Level value, std::format_string<Arguments...> pattern, Arguments&&... arguments) {
        if (!check(target, value)) return;
        const std::string text = std::format(pattern, std::forward<Arguments>(arguments)...);
        log(target, value, text);
    }

    template <typename... Arguments>
    static void fmt(const Type target, std::format_string<Arguments...> pattern, Arguments&&... arguments) {
        fmt(target, Level::Debug, pattern, std::forward<Arguments>(arguments)...);
    }

private:
    static inline auto mask = Type::All;
    static inline auto threshold = Level::Traceback;
    static inline bool ansi = false;
    static inline std::ofstream stream;
    static inline std::mutex mutex;

    static std::string_view name(Type target) noexcept;
    static std::string_view name(Level value) noexcept;
};