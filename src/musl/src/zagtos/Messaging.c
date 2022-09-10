#include <assert.h>
#include <zagtos/Messaging.h>
#include <zagtos/KernelApi.h>
#include <zagtos/syscall.h>

extern struct ZoProcessStartupInfo *__process_startup_info;

struct ZoMessageInfo *ZoGetRunMessage(void) {
    return &__process_startup_info->runMessage;
}


/* legacy */
void zagtos_send_message() {
}
void zagtos_create_port() {
}
void zagtos_spawn_process() {
}
