#include <zagtos/object.h>
#include <zagtos/messaging.h>

void zagtos_put_object(void *object) {
    ZObject *obj = (ZObject *)object;

    // TODO: write back changes

    zagtos_send_message(obj->info.environment, obj);
}
