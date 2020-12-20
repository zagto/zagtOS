#ifndef _UUID_H
#define _UUID_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char uuid_t[16];

extern int uuid_compare(const uuid_t a, const uuid_t b);
extern void uuid_copy(uuid_t destination, const uuid_t source);
extern void uuid_clear(uuid_t uuid);

#define UUID_DEFINE(name, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
    static const uuid_t name = {a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p}

#ifdef __cplusplus
}
#endif

#endif
