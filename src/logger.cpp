#include "logger.hpp"

#include <chrono>
#include <format>
#include <iostream>

void Logger::init(const int count, char** arguments) {
    for (int index = 1; index < count; ++index) {
        if (const std::string_view option(arguments[index]); option == "--debug" || option == "-d") {
            enable(Type::All);
        } else if (option.starts_with("--debug=")) {
            const std::string_view value = option.substr(8);
            if (value.contains("lexer"))     enable(Type::Lexer);
            if (value.contains("mouth"))     enable(Type::Mouth);
            if (value.contains("parser"))    enable(Type::Parser);
            if (value.contains("layout"))    enable(Type::Layout);
            if (value.contains("memory"))    enable(Type::Memory);
            if (value.contains("semantics")) enable(Type::Semantics);
            if (value.contains("all"))       enable(Type::All);
        } else if (option.starts_with("--log-file=")) {
            file(std::string(option.substr(11)));
        } else if (option == "--no-color") {
            color(false);
        }
    }
}

void Logger::types(const Type target) noexcept {
    const std::lock_guard guard(mutex);
    mask = target;
}

void Logger::enable(const Type target) noexcept {
    const std::lock_guard guard(mutex);
    mask |= target;
}

void Logger::disable(const Type target) noexcept {
    const std::lock_guard guard(mutex);
    mask &= ~target;
}

void Logger::level(const Level value) noexcept {
    const std::lock_guard guard(mutex);
    threshold = value;
}

void Logger::color(const bool flag) noexcept {
    const std::lock_guard guard(mutex);
    ansi = flag;
}

void Logger::file(const std::string& path) {
    const std::lock_guard guard(mutex);
    if (stream.is_open()) {
        stream.close();
    }
    stream.open(path, std::ios::out | std::ios::app);
}

void Logger::close() {
    const std::lock_guard guard(mutex);
    if (stream.is_open()) {
        stream.close();
    }
}

bool Logger::check(const Type target, const Level value) noexcept {
    if (static_cast<std::uint8_t>(value) < static_cast<std::uint8_t>(threshold)) {
        return false;
    }
    return (static_cast<std::uint32_t>(mask & target) != 0);
}

void Logger::log(const Type target, const Level value, const std::string_view text, const std::source_location& location) {
    if (!check(target, value)) return;

    const std::lock_guard guard(mutex);

    const auto now = std::chrono::system_clock::now();
    const std::string stamp = std::format("{:%H:%M:%S}", std::chrono::floor<std::chrono::seconds>(now));
    const std::string_view tag = name(target);
    const std::string_view rank = name(value);

    std::string output;
    if (ansi && !stream.is_open()) {
        output = std::format("\033[90m[{}]\033[0m {}[{}]\033[0m {}[{}]\033[0m {}\n",
                             stamp, style(target), tag, style(value), rank, text);
    } else {
        output = std::format("[{}] [{}] [{}] {}\n", stamp, tag, rank, text);
    }

    std::clog << output;
    if (stream.is_open()) {
        stream << output;
        stream.flush();
    }
}

std::string_view Logger::name(const Type target) noexcept {
    switch (target) {
        case Type::Lexer:     return "Lexer";
        case Type::Mouth:     return "Mouth";
        case Type::Parser:    return "Parser";
        case Type::Layout:    return "Layout";
        case Type::Memory:    return "Memory";
        case Type::Semantics: return "Semantics";
        case Type::All:       return "All";
        default:              return "General";
    }
}

std::string_view Logger::name(const Level value) noexcept {
    switch (value) {
        case Level::Trace: return "Trace";
        case Level::Debug: return "Debug";
        case Level::Info:  return "Info";
        case Level::Warn:  return "Warn";
        case Level::Error: return "Error";
        default:           return "Log";
    }
}

std::string_view Logger::style(const Type target) noexcept {
    switch (target) {
        case Type::Lexer:     return "\033[36m";
        case Type::Mouth:     return "\033[35m";
        case Type::Parser:    return "\033[33m";
        case Type::Layout:    return "\033[32m";
        case Type::Memory:    return "\033[34m";
        case Type::Semantics: return "\033[96m";
        default:              return "\033[37m";
    }
}

std::string_view Logger::style(const Level value) noexcept {
    switch (value) {
        case Level::Trace: return "\033[90m";
        case Level::Debug: return "\033[37m";
        case Level::Info:  return "\033[32m";
        case Level::Warn:  return "\033[33m";
        case Level::Error: return "\033[31m";
        default:           return "\033[0m";
    }
}