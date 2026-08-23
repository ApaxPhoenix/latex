#pragma once

namespace render {
    namespace layout {
        class Node;
    }

    class Wasm {
    public:
        void compose(const layout::Node* node, int width, int height);
        [[nodiscard]] int width() const;
        [[nodiscard]] int height() const;

    private:
        int width_{0};
        int height_{0};
    };
}