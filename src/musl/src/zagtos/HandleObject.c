#include <assert.h>
#include <zagtos/HandleObject.h>
#include <zagtos/syscall.h>

void ZoDeleteHandle(int handle) {
    if (handle != INVALID_HANDLE) {
        zagtos_syscall(SYS_DELETE_HANDLE, handle);
    }
}
