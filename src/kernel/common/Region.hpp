#pragma once

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
    bool isPageAligned() const {
        return !(start % PAGE_SIZE || length % PAGE_SIZE);
    }
    bool overlaps(Region otherRegion) const {
        /* https://stackoverflow.com/questions/3269434/whats-the-most-efficient-way-to-test-
         * two-integer-ranges-for-overlap */
        return start < otherRegion.end() && otherRegion.start < end();
    }
    bool contains(Region otherRegion) const {
        return start <= otherRegion.start && end() >= otherRegion.end();
    }
};
