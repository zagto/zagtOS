#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <common/common.hpp>
#include <memory/Memory.hpp>
#include <log/logger.hpp>

template<typename ElementType> class Vector {
protected:
    ElementType *data{nullptr};
    usize numElements;
    usize numAllocated;

    void updateAllocatedSize() {
        data = Memory::instance()->resizeVirtualArea(KernelVirtualAddress(data),
                                                     numAllocated * sizeof(ElementType)).asPointer<ElementType>();
    }

public:
    Vector(usize numElements = 0) :
            numElements{numElements},
            numAllocated{numElements}  {

        data = Memory::instance()->allocateVirtualArea(
                    sizeof(ElementType) * numAllocated).asPointer<ElementType>();
    }

    ~Vector() {
        for (usize i = 0; i < numElements; i++) {
            data[i].~ElementType();
        }
    }

    ElementType &operator[](usize index) {
        Assert(index < numElements);
        return data[index];
    }

    usize size() {
        return numElements;
    }

    void pushBack(ElementType element) {
        if (numElements == numAllocated) {
            numAllocated = (numAllocated + 1) * 3 / 2;
        }
        updateAllocatedSize();

        data[numElements] = element;
        numElements++;
    }

    void insert(ElementType element, usize index) {
        if (numElements == numAllocated) {
            numAllocated = (numAllocated + 1) * 3 / 2;
        }
        updateAllocatedSize();

        Assert(index <= numElements);
        memmove(&data[index+1], &data[index], (numElements - index) * sizeof(ElementType));

        data[index] = element;
        numElements++;
    }

    void remove(ElementType element) {
        usize index;
        for (index = 0; index < numElements; index++) {
            if (data[index] == element) {
                break;
            }
        }

        if (index == numElements) {
            log::Log << "Tried to remove Element that is not in vector" << log::EndLine;
            Panic();
        }

        numElements--;
        memmove(&data[index], &data[index+1], (numElements - index) * sizeof(ElementType));
    }
};

#endif // VECTOR_HPP
