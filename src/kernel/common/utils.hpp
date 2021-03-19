#pragma once

#include <common/inttypes.hpp>

#ifdef __cplusplus
#include <common/addresses.hpp>
#include <setup/HandOverState.hpp>

enum class AlignDirection {
    DOWN,
    UP
};

using Permissions = hos_v1::Permissions;

/* move implementation
 * https://stackoverflow.com/questions/53604753/how-does-one-force-a-c-move-operator-without-
 * stdwhatever?noredirect=1&lq=1 */
template<typename T>
struct remove_reference {
    typedef T type;
};
template<typename T>
struct remove_reference<T &> {
    typedef T type;
};
template<typename T>
struct remove_reference<T &&> {
    typedef T type;
};
template<typename T>
typename remove_reference<T>::type&& move (T &&original) {
    return static_cast<typename remove_reference<T>::type &&>(original);
}

template<typename T>
void swap(T &a, T&b) {
    T tmp = (a);
    a = move(b);
    b = move(tmp);
}

struct timespec {
    uint64_t tv_sec;
    uint32_t tv_nsec;
};

/* Loader may be linked against GNU EFI, which also contains these symbols. These macros avoid this
 * conflict by renaming them. */
#ifdef ZAGTOS_LOADER
#define memset loader_memset
#define memcpy loader_memcpy
#endif

// used by dlmalloc
extern "C" {
#endif
    void *memset(void *pointer, int value, size_t len);
    void *memcpy(void *dest, const void *src, size_t len);
    void *memmove(void *dest, const void *src, size_t len);
#ifdef __cplusplus
}

size_t align(size_t address, size_t alignment, AlignDirection direction);
void alignedShrink(size_t &start, size_t &length, size_t alignment);
void alignedGrow(size_t &start, size_t &length, size_t alignment);

template <typename T> T min(T a, T b) {
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

template <typename T> T max(T a, T b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

#ifndef ZAGTOS_LOADER
class Status;
namespace std {
    enum class align_val_t: size_t {};
}
void *operator new(size_t, KernelVirtualAddress address);
void *operator new[](size_t, Status &status);
void *operator new[](size_t, std::align_val_t align,  Status &status);
#endif

#endif // __cplusplus
