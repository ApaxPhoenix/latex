#include "allocator.hpp"
#include <cassert>

int main() {
    sandbox::Allocator allocator(1024);

    void* block = allocator.allocate(256);
    assert(block != nullptr);

    allocator.deallocate(block, 256);

    return 0;
}