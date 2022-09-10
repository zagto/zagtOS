#include <assert.h>
#include <zagtos/SharedMemory.h>
#include <zagtos/syscall.h>

int ZoCreateDMASharedMemory(size_t deviceMax, size_t length, size_t *deviceAddresses) {
    return zagtos_syscall(SYS_CREATE_SHARED_MEMORY, 2, deviceMax, length, deviceAddresses);
}

int ZoCreatePhysicalSharedMemory(size_t physicalAddress, size_t length) {
    return zagtos_syscall(SYS_CREATE_SHARED_MEMORY, 1, physicalAddress, length, 0);
}

int ZoCreateStandardSharedMemory(size_t length) {
    return zagtos_syscall(SYS_CREATE_SHARED_MEMORY, 0, 0, length, 0);
}

void ZoUnmapWhole(void *pointer) {
    size_t ret = zagtos_syscall(SYS_MUNMAP, 1, pointer, 0);
    assert(ret == 0);
}
