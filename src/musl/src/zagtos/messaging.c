#include <zagtos/messaging.h>
#include <zagtos/syscall.h>

_Bool zagtos_send_message(ZUUID target_port, void *obj) {
    return zagtos_uuid_syscall_1_1(SYS_SEND_MESSAGE, target_port, obj);
}

ZMessagePort zagtos_create_port(ZUUID allowed_type, ZUUID allowed_sender_tag) {
    struct {
        ZUUID allowed_type;
        ZUUID allowed_sender_tag;
        ZUUID *result;
    } call_data;

    ZMessagePort msg_port;

    call_data.allowed_type = allowed_type;
    call_data.allowed_sender_tag = allowed_sender_tag;
    call_data.result = &msg_port.uuid;
    zagtos_syscall(SYS_CREATE_PORT, &call_data);
    return msg_port;
}
