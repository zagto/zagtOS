#pragma once

#include <common/inttypes.hpp>
#include <common/panic.hpp>


class Region {
public:
    size_t start;
    size_t length;

    constexpr Region() noexcept :
        start{0},
        length{0} {}
    constexpr Region(size_t start, size_t length) noexcept :
            start{start},
            length{length} {}
    size_t end() const noexcept {
        assert(start + length >= start);
        return start + length;
    }
    bool isPageAligned() const {
        return !(start % PAGE_SIZE || length % PAGE_SIZE);
    }
    bool overlaps(Region otherRegion) const noexcept {
        /* https://stackoverflow.com/questions/3269434/whats-the-most-efficient-way-to-test-
         * two-integer-ranges-for-overlap */
        return start < otherRegion.end() && otherRegion.start < end();
    }
    bool contains(Region otherRegion) const noexcept {
        return start <= otherRegion.start && end() >= otherRegion.end();
    }
    bool contains(size_t value) const noexcept {
        return start <= value && end() > value;
    }
    void merge(Region otherRegion) noexcept {
        size_t newEnd = end() > otherRegion.end() ? end() : otherRegion.end();
        start = start < otherRegion.start ? start : otherRegion.start;
        assert(newEnd > start);
        length = newEnd - start;
    }
};
