#ifndef SORTEDVECTOR_HPP
#define SORTEDVECTOR_HPP

#include <lib/vector.hpp>

template<typename ElementType, bool Comperator(ElementType, ElementType)>
class SortedVector : public vector<ElementType> {
protected:
    size_t findIndexFor(ElementType element) {
        size_t low = 0;
        size_t high = this->numElements;
        while (low < high) {
            size_t index = low + (high - low) / 2;
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
    void push_back(ElementType) {
        cout << "pushBack on sorted vector" << endl;
        Panic();
    }

    void insert(ElementType, size_t) {
        cout << "indexed insert on sorted vector" << endl;
        Panic();
    }

    void insert(ElementType element) {
        static_cast<vector<ElementType> *>(this)->insert(element, findIndexFor(element));
    }
};


#endif // SORTEDVECTOR_HPP
