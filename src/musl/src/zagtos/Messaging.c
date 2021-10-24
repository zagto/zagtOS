#include <assert.h>
#include <zagtos/Messaging.h>
#include <zagtos/syscall.h>

struct ZoMessageInfo *__run_message;

int ZoCreateDMASharedMemory(size_t deviceMax, size_t length, size_t *deviceAddresses) {
    return zagtos_syscall(SYS_CREATE_SHARED_MEMORY, 2, deviceMax, length, deviceAddresses);
}

int ZoCreatePhysicalSharedMemory(size_t physicalAddress, size_t length) {
    return zagtos_syscall(SYS_CREATE_SHARED_MEMORY, 1, physicalAddress, length, 0);
}

int ZoCreateStandardSharedMemory(size_t length) {
    return zagtos_syscall(SYS_CREATE_SHARED_MEMORY, 0, 0, length, 0);
}

void ZoDeleteHandle(int handle) {
    if (handle != INVALID_HANDLE) {
        zagtos_syscall(SYS_DELETE_HANDLE, handle);
    }
}

void ZoUnmapWhole(void *pointer) {
    size_t ret = zagtos_syscall(SYS_MUNMAP, 1, pointer, 0);
    assert(ret == 0);
}

struct ZoMessageInfo *ZoGetRunMessage(void) {
    return __run_message;
}


/* legacy */
void zagtos_send_message() {
}
void zagtos_create_port() {
}
void zagtos_spawn_process() {
}
