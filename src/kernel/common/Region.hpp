#ifndef REGION_HPP
#define REGION_HPP

#include <common/inttypes.hpp>
#include <common/panic.hpp>


class Region {
public:
    size_t start;
    size_t length;

    constexpr Region() :
        start{0},
        length{0} {}
    constexpr Region(size_t start, size_t length) :
            start{start},
            length{length} {}
    size_t end() const {
        assert(start + length >= start);
        return start + length;
    }
    bool isPageAligned() {
        return !(start % PAGE_SIZE || length % PAGE_SIZE);
    }
};

#endif // REGION_HPP
