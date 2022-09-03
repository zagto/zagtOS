#include <stdlib.h>
#include <assert.h>
#include <sys/random.h>
#include <zagtos/object.h>
#include <zagtos/messaging-legacy.h>


void zagtos_put_object(void *object) {
    ZObject *obj = (ZObject *)object;

    // TODO: write back changes

    zagtos_send_message(obj->info.environment, obj);
}


_Bool zagtos_is_object_writeable(ZObject *obj) {
    return obj->info.flags & ZOBJECT_WRITEABLE;
}


_Bool zagtos_read_object_info(ZUUID id, ZObjectInfo *info) {
    /* TODO: implement */
    assert(0);
}


ZObject *zagtos_create_object(ZUUID type, uint64_t size) {
    ZObject *object = calloc(size, 1);
    if (object == NULL) {
        return NULL;
    }

    getrandom(&object->info.id, sizeof(ZUUID), 0);
    object->info.type = type;
    object->info.environment = LOCAL_ENVIRONMENT;
    object->info.num_references = 1;
    object->info.num_data_bytes = size - sizeof(ZObject);

    /* TODO: implement */
    assert(0);
}
