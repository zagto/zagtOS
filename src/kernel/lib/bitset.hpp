#ifndef BITSET_HPP
#define BITSET_HPP

#include <common/inttypes.hpp>
#include <common/panic.hpp>

template<size_t numBits> class bitset {
    class reference {
    private:
        bitset<numBits> &set;
        size_t position;

    public:
        reference(bitset<numBits> &set, size_t position):
            set{set}, position{position} {}
        operator bool() const {
            return set.test(position);
        }
        reference &operator=(bool newValue) {
            if (newValue) {
                set.set(position);
            } else {
                set.reset(position);
            }
            return *this;
        }
    };

    static const size_t BITS_PER_ELEMENT = sizeof(size_t) * 8;
    static const size_t NUM_ELEMENTS = (numBits + BITS_PER_ELEMENT - 1) / BITS_PER_ELEMENT;
    size_t data[NUM_ELEMENTS] = {0};

    bool test(size_t position) {
        assert(position < NUM_ELEMENTS);
        return (data[position / BITS_PER_ELEMENT] >> (position % BITS_PER_ELEMENT)) & 1;
    }
    void set(size_t position) {
        assert(position < NUM_ELEMENTS);
        data[position / BITS_PER_ELEMENT] |= 1 << (position % BITS_PER_ELEMENT);
    }
    void reset(size_t position) {
        assert(position < NUM_ELEMENTS);
        data[position / BITS_PER_ELEMENT] &= ~(1 << (position % BITS_PER_ELEMENT));
    }
    reference operator[](size_t position) {
        return reference(*this, position);
    }
};

#endif // BITSET_HPP
