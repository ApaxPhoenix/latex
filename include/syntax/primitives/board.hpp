#pragma once

#include <array>
#include <fstream>
#include <string>
#include <string_view>

namespace syntax::primitives::streams {

    constexpr std::size_t maximum = 16uz;

    class Reader {
    public:
        void open(std::size_t index, std::string_view filename);
        void close(std::size_t index);
        bool read(std::size_t index, std::string& content);
        [[nodiscard]] bool finished(std::size_t index) const noexcept;

    private:
        struct File {
            std::ifstream stream;
            bool opened = false;
        };
        std::array<File, maximum> files{};
    };

    class Writer {
    public:
        void open(std::size_t index, std::string_view filename);
        void close(std::size_t index);
        void write(std::size_t index, std::string_view content);

    private:
        struct File {
            std::ofstream stream;
            bool opened = false;
        };
        std::array<File, maximum> files{};
    };

    class Board {
    public:
        Reader reader;
        Writer writer;
    };

    Board& board() noexcept;

}