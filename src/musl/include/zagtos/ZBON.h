#ifndef __ZAGTOS_ZBON_H
#define __ZAGTOS_ZBON_H

#include <stdint.h>
#include <stddef.h>

struct ZbonEncodedData {
    uint8_t *data;
    size_t size;
    size_t numHandles;
    _Bool allocatedExternally;
};

#endif
