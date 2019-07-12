#ifndef SORTEDVECTOR_HPP
#define SORTEDVECTOR_HPP

#include <lib/Vector.hpp>

template<typename ElementType> class SortedVector : public Vector<ElementType> {
private:
    usize findIndexFor(ElementType element) {
        usize low = 0;
        usize high = this->numElements;
        while (low + 1 < high) {
            usize index = low + (high - low) / 2;
            if (element < this->data[index]) {
                high = index;
            } else {
                low = index;
            }
        }
        return low;
    }


public:
    void pushBack(ElementType) {
        Log << "pushBack on sorted vector" << EndLine;
        Panic();
    }

    void insert(ElementType, usize) {
        Log << "indexed insert on sorted vector" << EndLine;
        Panic();
    }

    void insert(ElementType element) {
        static_cast<Vector<ElementType> *>(this)->insert(element, findIndexFor(element));
    }
};

#endif // SORTEDVECTOR_HPP
