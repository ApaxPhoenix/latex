#include "syntax/primitives/board.hpp"

namespace syntax::primitives::streams {

    void Reader::open(const std::size_t index, const std::string_view filename) {
        if (index >= maximum) return;
        files[index].stream.open(std::string(filename));
        files[index].opened = files[index].stream.is_open();
    }

    void Reader::close(const std::size_t index) {
        if (index >= maximum) return;
        files[index].stream.close();
        files[index].opened = false;
    }

    bool Reader::read(const std::size_t index, std::string& content) {
        if (index >= maximum || !files[index].opened) return false;
        return static_cast<bool>(std::getline(files[index].stream, content));
    }

    bool Reader::finished(const std::size_t index) const noexcept {
        if (index >= maximum || !files[index].opened) return true;
        return files[index].stream.eof();
    }

    void Writer::open(const std::size_t index, const std::string_view filename) {
        if (index >= maximum) return;
        files[index].stream.open(std::string(filename), std::ios::app);
        files[index].opened = files[index].stream.is_open();
    }

    void Writer::close(const std::size_t index) {
        if (index >= maximum) return;
        files[index].stream.close();
        files[index].opened = false;
    }

    void Writer::write(const std::size_t index, const std::string_view content) {
        if (index >= maximum || !files[index].opened) return;
        files[index].stream << content << "\n";
    }

    Board& board() noexcept {
        static Board instance;
        return instance;
    }

}