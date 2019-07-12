#ifndef INTTYPES_HPP
#define INTTYPES_HPP

typedef char i8;
typedef short i16;
typedef int i32;
typedef long i64;
typedef __int128_t i128;
typedef i64 isize;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;
typedef __uint128_t u128;
typedef u64 usize;

static const usize PAGE_SIZE = 0x1000;

#endif // INTTYPES_HPP
