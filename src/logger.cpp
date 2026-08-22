#include "logger.hpp"

#include <chrono>
#include <format>
#include <iostream>

void Logger::init(const int count, char** arguments) {
    bool selective = false;

    for (int index = 1; index < count; ++index) {
        if (const std::string_view option(arguments[index]); option == "--debug" || option == "-d") {
            types(Type::All);
        } else if (option.starts_with("--debug=")) {
            if (!selective) {
                types(Type::None);
                selective = true;
            }
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

    const std::string output = std::format("[{}] [{}] [{}] {}\n", stamp, tag, rank, text);

    std::clog << output << std::flush;
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
        case Level::Traceback:   return "Traceback";
        case Level::Debug:       return "Debug";
        case Level::Informative: return "Informative";
        case Level::Warning:     return "Warning";
        case Level::Error:       return "Error";
        default:                 return "Log";
    }
}