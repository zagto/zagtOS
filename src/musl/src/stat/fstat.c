#define _BSD_SOURCE
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <zagtos/unixcompat.h>
#include "__stat_common.h"

int fstat(int fd, struct stat *st)
{
    ZFileDescriptor *zfd = zagtos_get_file_descriptor_object(fd);
    if (!zfd) {
        errno = EBADF;
        return -1;
    }

    return __stat_common(&zfd->object->info, st);
}

#if !_REDIR_TIME64
weak_alias(fstat, fstat64);
#endif
