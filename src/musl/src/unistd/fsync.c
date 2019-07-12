#include <unistd.h>
#include <errno.h>
#include <zagtos/unixcompat.h>
#include "syscall.h"

int fsync(int fd)
{
    ZFileDescriptor *obj = zagtos_get_file_descriptor_object(fd);
    if (!obj) {
        errno = EBADF;
        return -1;
    }
    zagtos_sync_object(obj);
    return 0;
}
