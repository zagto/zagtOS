#ifndef REGION_HPP
#define REGION_HPP

#include <common/inttypes.hpp>
#include <common/panic.hpp>


class Region {
public:
    usize start;
    usize length;

    constexpr Region(usize start, usize length) :
            start{start},
            length{length} {
        //Assert(start + length >= start);
    }
    usize end() const {
        return start + length;
    }
    bool isPageAligned() {
        return !(start % PAGE_SIZE || length % PAGE_SIZE);
    }
};

#endif // REGION_HPP
