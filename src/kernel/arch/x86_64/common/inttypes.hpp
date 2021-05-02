#pragma once

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;
typedef __int128_t int128_t;
typedef int64_t ssize_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
typedef __uint128_t uint128_t;
typedef uint64_t size_t;

typedef uint64_t FutexFrameID;

static const size_t PAGE_SIZE = 0x1000;
static const size_t PAGE_SHIFT = 12u;
static const size_t PLATFORM_BITS = 64;
