#include <assert.h>
#include <zagtos/Messaging.h>
#include <zagtos/syscall.h>

int ZoCreatePhysicalSharedMemory(size_t physicalAddress, size_t length) {
    return zagtos_syscall(SYS_CREATE_SHARED_MEMORY, 1, physicalAddress, length);
}

int ZoCreateSharedMemory(size_t length) {
    return zagtos_syscall(SYS_CREATE_SHARED_MEMORY, 0, 0, length);
}

void ZoDeleteHandle(int handle) {
    if (handle != INVALID_HANDLE) {
        zagtos_syscall(SYS_DELETE_HANDLE, handle);
    }
}

void ZoUnmapWhole(void *pointer) {
    int ret = zagtos_syscall(SYS_MUNMAP, 1, pointer, 0);
    assert(ret == 0);
}


/* legacy */
void zagtos_send_message() {
}
void zagtos_create_port() {
}
void zagtos_spawn_process() {
}
struct zagtos_run_message_info *__run_message;
struct zagtos_run_message_info *zagtos_get_run_message(void) {
    return __run_message;
}
