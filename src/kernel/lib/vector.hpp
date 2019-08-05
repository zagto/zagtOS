#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <common/common.hpp>
#include <memory/Memory.hpp>
#include <log/logger.hpp>

template<typename ElementType> class vector {
protected:
    ElementType *data{nullptr};
    size_t numElements;
    size_t numAllocated;

    void updateAllocatedSize() {
        data = Memory::instance()->resizeVirtualArea(KernelVirtualAddress(data),
                                                     numAllocated * sizeof(ElementType)).asPointer<ElementType>();
    }

public:
    vector(size_t numElements = 0) :
            numElements{numElements},
            numAllocated{numElements}  {

        data = Memory::instance()->allocateVirtualArea(
                    sizeof(ElementType) * numAllocated).asPointer<ElementType>();
    }

    ~vector() {
        for (size_t i = 0; i < numElements; i++) {
            data[i].~ElementType();
        }
    }

    ElementType &operator[](size_t index) {
        assert(index < numElements);
        return data[index];
    }

    size_t size() {
        return numElements;
    }

    void push_back(ElementType element) {
        if (numElements == numAllocated) {
            numAllocated = (numAllocated + 1) * 3 / 2;
        }
        updateAllocatedSize();

        data[numElements] = element;
        numElements++;
    }

    void insert(ElementType element, size_t index) {
        if (numElements == numAllocated) {
            numAllocated = (numAllocated + 1) * 3 / 2;
        }
        updateAllocatedSize();

        assert(index <= numElements);
        memmove(&data[index+1], &data[index], (numElements - index) * sizeof(ElementType));

        data[index] = element;
        numElements++;
    }

    void remove(ElementType element) {
        size_t index;
        for (index = 0; index < numElements; index++) {
            if (data[index] == element) {
                break;
            }
        }

        if (index == numElements) {
            log::cout << "Tried to remove Element that is not in vector" << log::endl;
            Panic();
        }

        numElements--;
        memmove(&data[index], &data[index+1], (numElements - index) * sizeof(ElementType));
    }
};

#endif // VECTOR_HPP
