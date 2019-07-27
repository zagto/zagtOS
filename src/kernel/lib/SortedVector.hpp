#ifndef SORTEDVECTOR_HPP
#define SORTEDVECTOR_HPP

#include <lib/Vector.hpp>

template<typename ElementType, bool Comperator(ElementType, ElementType)>
class SortedVector : public Vector<ElementType> {
protected:
    usize findIndexFor(ElementType element) {
        usize low = 0;
        usize high = this->numElements;
        while (low < high) {
            usize index = low + (high - low) / 2;
            if (Comperator(element, this->data[index])) {
                if (index == low) {
                    return low;
                }
                high = index - 1;
            } else {
                low = index + 1;
            }
        }
        Assert(low == high);
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
