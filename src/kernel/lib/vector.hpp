#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <common/common.hpp>
#include <memory/Memory.hpp>
#include <log/Logger.hpp>

template<typename ElementType> class vector {
protected:
    ElementType *_data{nullptr};
    size_t numElements;
    size_t numAllocated;

    void updateAllocatedSize() {
        _data = Memory::instance()->resizeVirtualArea(KernelVirtualAddress(_data),
                                                     numAllocated * sizeof(ElementType)).asPointer<ElementType>();
    }

public:
    vector(size_t numElements = 0) :
            numElements{numElements},
            numAllocated{numElements}  {

        _data = Memory::instance()->allocateVirtualArea(
                    sizeof(ElementType) * numAllocated).asPointer<ElementType>();
    }

    ~vector() {
        for (size_t i = 0; i < numElements; i++) {
            _data[i].~ElementType();
        }
    }

    ElementType &operator[](size_t index) {
        assert(index < numElements);
        return _data[index];
    }

    size_t size() {
        return numElements;
    }

    void push_back(ElementType element) {
        if (numElements == numAllocated) {
            numAllocated = (numAllocated + 1) * 3 / 2;
        }
        updateAllocatedSize();

        _data[numElements] = element;
        numElements++;
    }

    void insert(ElementType element, size_t index) {
        if (numElements == numAllocated) {
            numAllocated = (numAllocated + 1) * 3 / 2;
        }
        updateAllocatedSize();

        assert(index <= numElements);
        memmove(&_data[index+1], &_data[index], (numElements - index) * sizeof(ElementType));

        _data[index] = element;
        numElements++;
    }

    void remove(ElementType element) {
        size_t index;
        for (index = 0; index < numElements; index++) {
            if (_data[index] == element) {
                break;
            }
        }

        if (index == numElements) {
            cout << "Tried to remove Element that is not in vector" << endl;
            Panic();
        }

        numElements--;
        memmove(&_data[index], &_data[index+1], (numElements - index) * sizeof(ElementType));
    }

    ElementType *data() {
        return _data;
    }
};

#endif // VECTOR_HPP
