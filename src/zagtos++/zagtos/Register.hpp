#pragma once

#include <cstdint>
#include <cassert>

#define BITS(name, shift, num) \
    uint32_t name() { \
        static_assert((shift) + (num) <= sizeof(uint32_t) * 8); \
        static_assert((num) > 0); \
        __sync_synchronize(); \
        return (__register_data >> (shift)) & ((1ul << (num)) - 1); \
    } \
    void name(uint32_t value) { \
        assert(value < (1ul << (num))); \
        __sync_synchronize(); \
        uint32_t keep = __register_data & ~(((1ul << (num)) - 1) << (shift)); \
        __register_data = keep | ((value) << (shift)); \
        __sync_synchronize(); \
    }

#define BIT(name, shift) BITS(name, shift, 1)

#define REGISTER(name, ...) \
    class { \
    private: \
        uint32_t __register_data; \
    public: \
        uint32_t operator()() { \
            __sync_synchronize(); \
            return __register_data; \
        } \
        void operator()(uint32_t value) { \
            __register_data = value; \
            __sync_synchronize(); \
        } \
        __VA_ARGS__ \
    } name;

#define BITSI(name, index, shift, num) \
    uint32_t name() { \
        static_assert((shift) + (num) <= sizeof(uint32_t) * 8); \
        static_assert((num) > 0); \
        __sync_synchronize(); \
        return (__register_data[index] >> (shift)) & ((1ul << (num)) - 1); \
    } \
    void name(uint32_t value) { \
        assert(value < (1ul << (num))); \
        __sync_synchronize(); \
        uint32_t keep = __register_data[index] & ~(((1ul << (num)) - 1) << (shift)); \
        __register_data[index] = keep | ((value) << (shift)); \
        __sync_synchronize(); \
    }

#define BITI(name, index, shift) BITSI(name, index, shift, 1)

#define REGISTERSI(count, ...) \
    uint32_t atIndex(size_t index) { \
        __sync_synchronize(); \
        return __register_data[index]; \
    } \
    void atIndex(size_t index, uint32_t value) { \
        __register_data[index] = value; \
        __sync_synchronize(); \
    } \
    uint32_t __register_data[count]; \
    __VA_ARGS__ \
