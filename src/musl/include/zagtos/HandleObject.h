#ifndef __ZAGTOS_HANDLE_OBJECT_H
#define __ZAGTOS_HANDLE_OBJECT_H

#include <stdint.h>

static const uint32_t INVALID_HANDLE = (uint32_t)-1;

void ZoDeleteHandle(int handle);

#endif
