#include <unistd.h>
#include <errno.h>
#include <zagtos/unixcompat.h>
#include "syscall.h"

int ftruncate(int fd, off_t length)
{
    ZObject *obj = (ZObject *)zagtos_get_file_descriptor_object(fd);
    if (!obj) {
        errno = EBADF;
        return -1;
    }
    zagtos_resize_object(obj, length);
    return 0;
}

weak_alias(ftruncate, ftruncate64);
