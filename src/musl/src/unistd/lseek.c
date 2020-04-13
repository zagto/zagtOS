#include <unistd.h>
#include <errno.h>
#include <zagtos/unixcompat.h>
#include "syscall.h"
#include "lock.h"

off_t __lseek(int fd, off_t offset, int whence)
{
    ZFileDescriptor *zfd = zagtos_get_file_descriptor_object(fd);
    if (!zfd) {
        errno = EBADF;
        return -1;
    }
    LOCK(&zfd->lock);
    if (whence == SEEK_SET) {
        if (offset < 0) {
            errno = EINVAL;
            goto fail;
        }
        zfd->position = offset;
    } else if (whence == SEEK_CUR) {
        if (offset < 0 && zfd->position < -offset) {
            errno = EINVAL;
            goto fail;
        }
        zfd->position += offset;
    } else if (whence == SEEK_END) {
        zfd->position = zfd->object->info.num_data_bytes + offset;
    } else {
        errno = EINVAL;
        goto fail;
    }

    UNLOCK(&zfd->lock);
    return 0;

fail:
    UNLOCK(&zfd->lock);
    return -1;
}

weak_alias(__lseek, lseek);
weak_alias(__lseek, lseek64);
