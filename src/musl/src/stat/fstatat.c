#define _BSD_SOURCE
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <zagtos/filesystem.h>
#include <zagtos/unixcompat.h>
#include "__stat_common.h"

int fstatat(int dir_fd, const char *restrict path, struct stat *restrict buf, int flag)
{
    /* allow the flag, but it is ignored as we have no symlinks */
    if (flag & ~AT_SYMLINK_NOFOLLOW) {
        errno = EINVAL;
        return -1;
    }

    ZDirectory *dir = NULL;

    if (dir_fd != AT_FDCWD && path[0] != '/') {
        ZFileDescriptor *zfd = zagtos_get_file_descriptor_object(dir_fd);
        if (!zfd) {
            return -1;
        }
        if (!zagtos_compare_uuid(zfd->object->info.type, TYPE_DIRECTORY)) {
            errno = ENOTDIR;
            return -1;
        }
        dir = (ZDirectory *)zfd->object;
    }

    ZObjectInfo *obj_info = zagtos_get_object_info_by_path(path, dir);
    if (!obj_info) {
        errno = ENOENT;
        return -1;
    }

    int result = __stat_common(obj_info, buf);
    zagtos_put_object_info(obj_info);
    return result;
}

#if !_REDIR_TIME64
weak_alias(fstatat, fstatat64);
#endif
