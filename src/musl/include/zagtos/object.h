#ifndef __ZAGTOS_OBJECT_H
#define __ZAGTOS_OBJECT_H

#include <zagtos/uuid.h>

typedef struct {
    ZUUID id;
    ZUUID type;
    ZUUID environment;
    uint64_t num_references;
    uint64_t num_data_bytes;
    uint64_t flags;
} ZObjectInfo;

static const uint64_t ZOBJECT_WRITEABLE = 0x1,
                      ZOBJECT_MODIFIED = 0x2;

typedef struct {
    ZObjectInfo info;
    void *data;
} ZObject;

ZObject *zagtos_create_object(ZUUID type, uint64_t size);
_Bool zagtos_read_object_info(ZUUID id, ZObjectInfo *info);
void zagtos_sync_object(void *object);
void zagtos_resize_object(void *object, uint64_t new_size);
void zagtos_put_object(void *object);
void zagtos_put_object_info(ZObjectInfo *objectInfo);
_Bool zagtos_is_object_writeable(ZObject *obj);

static const ZUUID TYPE_UNSPECIFIED = {0x325e9ab697554342, 0xbb66d89312760893};

static const ZUUID UNREFERENCE_OBJECT_MSG = {0x2450a77820074f68, 0xaee088284f01857c};


#endif // OBJECT_H
