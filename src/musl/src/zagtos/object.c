#include <zagtos/object.h>
#include <zagtos/messaging.h>
#include <assert.h>


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
    /* TODO: implement */
    assert(0);
}
