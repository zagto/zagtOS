#include <common/common.hpp>
#include <paging/PageTable.hpp>
#include <system/System.hpp>


void memset(void *pointer, char value, usize len)
{
    for (usize i = 0; i < len; i++)
        static_cast<char *>(pointer)[i] = value;
}

void memcpy(void *dest, const void *src, usize len)
{
    for (usize i = 0; i < len; i++)
        static_cast<u8 *>(dest)[i] = static_cast<const u8 *>(src)[i];
}

void memmove(void *dest, const void *src, usize len)
{
    if (dest < src)
    {
        for (usize i = 0; i < len; i++)
            static_cast<u8 *>(dest)[i] = static_cast<const u8 *>(src)[i];
    }
    else if (dest > src)
    {
        for (usize i = len; i > 0; i--)
            static_cast<u8 *>(dest)[i-1] = static_cast<const u8 *>(src)[i-1];
    }
}


usize align(usize address, usize alignment, AlignDirection direction)
{
    if (direction == AlignDirection::UP)
        return address % alignment ? address + alignment - address % alignment : address;
    else
        return address - address % alignment;
}

void alignedShrink(usize &start, usize &length, usize alignment)
{
    usize newStart = align(start, alignment, AlignDirection::UP);
    if (length < newStart - start)
        length = newStart; // TODO : 0????
    else
    {
        length -= newStart - start;
        length = align(length, alignment, AlignDirection::DOWN);
    }
    start = newStart;
}


void alignedGrow(usize &start, usize &length, usize alignment)
{
    usize newStart = align(start, alignment, AlignDirection::DOWN);
    length += start - newStart;
    length = align(length, alignment, AlignDirection::UP);
    start = newStart;
}


void *operator new(usize, KernelVirtualAddress address) {
    return address.asPointer<void>();
}


void *operator new(usize size) {
    return CurrentSystem.memory.allocateVirtualArea(size).asPointer<void>();
}

namespace std {
    enum class align_val_t: usize {};
}

void *operator new(usize size, std::align_val_t align) {
    // HACK
    usize *align_ptr = reinterpret_cast<usize *>(&align);
    return CurrentSystem.memory.allocateVirtualArea(size, *align_ptr).asPointer<void>();
}


void *operator new[](usize size) {
    return operator new(size);
}


void operator delete(void *object) {
    CurrentSystem.memory.freeVirtualArea(KernelVirtualAddress(object));
}

void operator delete(void *object, usize size) {
    Assert(size <= PAGE_SIZE);
    operator delete(object);
}

void operator delete[](void *object) {
    operator delete(object);
}

void operator delete[](void *object, usize size) {
    Assert(size <= PAGE_SIZE);
    operator delete(object);
}
