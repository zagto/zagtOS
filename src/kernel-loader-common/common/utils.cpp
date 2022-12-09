#include <common/utils.hpp>
#include <iostream>

bool operator>(Permissions a, Permissions b) {
    assert(a != Permissions::INVALID);
    assert(b != Permissions::INVALID);

    switch (a) {
    case Permissions::READ_WRITE_EXECUTE:
        return b == Permissions::READ || b == Permissions::READ_WRITE || b == Permissions::READ_EXECUTE;
    case Permissions::READ_WRITE:
        return b == Permissions::READ;
    case Permissions::READ_EXECUTE:
        return b == Permissions::READ;
    case Permissions::READ:
        return false;
    default:
        cout << "not implemented permission" << endl;
        Panic();
    }
}

bool operator>=(Permissions a, Permissions b) {
    return a == b || a > b;
}

bool operator<(Permissions a, Permissions b) {
    return b > a;
}

bool operator<=(Permissions a, Permissions b) {
    return a == b || a < b;
}

#pragma GCC push_options
#pragma GCC optimize ("-O3")
#pragma GCC optimize ("-fno-tree-loop-distribute-patterns")

void *memset(void *pointer, int value, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        static_cast<uint8_t *>(pointer)[i] = static_cast<uint8_t>(value);
    }
    return pointer;
}

void *memcpy(void *dest, const void *src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        static_cast<uint8_t *>(dest)[i] = static_cast<const uint8_t *>(src)[i];
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t len) {
    if (dest < src) {
        for (size_t i = 0; i < len; i++)
            static_cast<uint8_t *>(dest)[i] = static_cast<const uint8_t *>(src)[i];
    } else if (dest > src) {
        for (size_t i = len; i > 0; i--)
            static_cast<uint8_t *>(dest)[i-1] = static_cast<const uint8_t *>(src)[i-1];
    }
    return dest;
}

char *strchr(const char *s, int c) {
    while (*s != '\0') {
        if (*s == c) {
            return const_cast<char *>(s);
        }
        s++;
    }
    return nullptr;
}

int memcmp(const void *_a, const void *_b, size_t length) {
    const uint8_t *a = reinterpret_cast<const uint8_t *>(_a);
    const uint8_t *b = reinterpret_cast<const uint8_t *>(_b);
    for (size_t index = 0; index < length; index++) {
        if (a[index] < b[index]) {
            return -1;
        }
        if (a[index] > b[index]) {
            return 1;
        }
    }
    return 0;
}

#pragma GCC pop_options

size_t align(size_t address, size_t alignment, AlignDirection direction) {
    if (direction == AlignDirection::UP) {
        return address % alignment ? address + alignment - address % alignment : address;
    } else {
        return address - address % alignment;
    }
}

void alignedShrink(size_t &start, size_t &length, size_t alignment) {
    size_t newStart = align(start, alignment, AlignDirection::UP);
    if (length < newStart - start) {
        length = 0;
    } else {
        length -= newStart - start;
        length = align(length, alignment, AlignDirection::DOWN);
    }
    start = newStart;
}


void alignedGrow(size_t &start, size_t &length, size_t alignment) {
    size_t newStart = align(start, alignment, AlignDirection::DOWN);
    length += start - newStart;
    length = align(length, alignment, AlignDirection::UP);
    start = newStart;
}
