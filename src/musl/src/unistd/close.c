#include <unistd.h>
#include <errno.h>
#include <zagtos/unixcompat.h>
#include "syscall.h"

int close(int fd)
{
    ZFileDescriptor *zfd = zagtos_get_file_descriptor_object(fd);
    zagtos_put_object(zfd->object);
    zagtos_free_file_descriptor(fd);
    return 0;
}
