#ifndef __ZAGTOS_UUID_H
#define __ZAGTOS_UUID_H

#include <stdint.h>

/* TODO: this file should be deleted and uuid/uuid.h should be used */

typedef struct {
    uint64_t data[2];
} ZUUID;

inline _Bool zagtos_compare_uuid(ZUUID a, ZUUID b) {
    return a.data[0] == b.data[0] && a.data[1] == b.data[1];
}

#endif // UUID_H
