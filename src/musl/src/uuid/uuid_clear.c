#include <string.h>
#include <uuid/uuid.h>

void uuid_clear(uuid_t uuid) {
    memset(uuid, 0, 16);
}
