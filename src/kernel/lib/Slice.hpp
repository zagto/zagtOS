#ifndef SLICE_HPP
#define SLICE_HPP

#include <common/inttypes.hpp>

template<template<typename> class ContainerType, typename ElementType> class Slice {
private:
    ContainerType<ElementType> *container;
    size_t offset;
    size_t _size;

public:
    Slice(ContainerType<ElementType> *container, size_t offset, size_t size) :
            container{container}, offset{offset}, _size{size} {
        Assert(offset + _size <= container->size() && offset + _size >= offset);
    }

    Slice(ContainerType<ElementType> *container) : Slice(container, 0, container->size()) {}

    Slice(Slice<ContainerType, ElementType> *bigSlice, size_t offset, size_t size) :
            container{bigSlice->container},
            offset{bigSlice->offset + offset},
            _size{size} {

        if (size == 0) {
            return;
        }
        Assert(offset + size < bigSlice->size());
        Assert(offset + size > offset);
    }

    ElementType &operator[](size_t index) {
        Assert(index < _size);
        return (*container)[index + offset];
    }

    size_t size() {
        return _size;
    }

    template<typename ResultType> ResultType interpretAsObject(size_t offset) {
        ResultType result;
        arrayCopy(reinterpret_cast<uint8_t *>(&result), *this, sizeof(ResultType), 0, offset);
        return result;
    }

    ContainerType<ElementType> copy() {
        ContainerType<ElementType> result(_size);
        arrayCopy(result, *container, offset, _size);
        return result;
    }
};

#endif // SLICE_HPP
