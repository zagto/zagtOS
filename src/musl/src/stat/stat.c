#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <zagtos/filesystem.h>
#include "syscall.h"
#include "__stat_common.h"

int stat(const char *restrict path, struct stat *restrict buf)
{
    ZObjectInfo *obj_info = zagtos_get_object_info_by_path(path, NULL);
    if (!obj_info) {
        errno = ENOENT;
        return -1;
    }

    int result = __stat_common(obj_info, buf);
    zagtos_put_object_info(obj_info);
    return result;
}

weak_alias(stat, stat64);
