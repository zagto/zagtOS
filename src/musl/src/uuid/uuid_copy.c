#include <string.h>
#include <uuid/uuid.h>

extern void uuid_copy(uuid_t destination, const uuid_t source) {
    memcpy(destination, source, 16);
}
