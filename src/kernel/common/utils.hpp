#ifndef UTILS_HPP
#define UTILS_HPP

#include <common/inttypes.hpp>

#ifdef __cplusplus
enum class AlignDirection {
    DOWN,
    UP
};

enum class Permissions {
    WRITE = 1, EXECUTE = 2, WRITE_AND_EXECUTE = 3, NONE = 4
};


// used by dlmalloc
extern "C" {
#endif
    void memset(void *pointer, char value, usize len);
    void memcpy(void *dest, const void *src, usize len);
    void memmove(void *dest, const void *src, usize len);
#ifdef __cplusplus
}

usize align(usize address, usize alignment, AlignDirection direction);
void alignedShrink(usize *start, usize *length, usize alignment);
void alignedGrow(usize *start, usize *length, usize alignment);

usize randomU32();
template <typename T> T random() {
    constexpr usize numRandom = (sizeof(T) - 1) / sizeof(u32) + 1;

    u32 r[numRandom];
    T result;
    memcpy(&result, r, sizeof(T));
    return result;
}

void *operator new(usize, KernelVirtualAddress address);

template<typename T1, typename T2> bool arrayCompare(T1 &a, T2 &b, usize length) {
    for (usize index = 0; index < length; index++) {
        if (a[index] != b[index]) {
            return false;
        }
    }
    return true;
}

template<typename T1, typename T2> void arrayCopy(T1 destination,
                                                  T2 source,
                                                  usize length,
                                                  usize destinationOffset = 0,
                                                  usize sourceOffset = 0) {
    for (usize index = 0; index < length; index++) {
        destination[index + destinationOffset] = source[index + sourceOffset];
    }
}

#endif // __cplusplus
#endif // UTILS_HPP
