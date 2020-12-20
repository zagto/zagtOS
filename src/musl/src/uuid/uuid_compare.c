#include <string.h>
#include <uuid/uuid.h>

extern int uuid_compare(const uuid_t a, const uuid_t b) {
    return memcmp(a, b, 16);
}
