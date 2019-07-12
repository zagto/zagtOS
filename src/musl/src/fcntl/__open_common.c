#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <zagtos/filesystem.h>
#include <zagtos/unixcompat.h>
#include "__open_common.h"

int __open_common(int dir_fd, const char *filename, int flags, mode_t mode)
{
    if (flags & ~(O_EXEC | O_RDWR | O_WRONLY | O_EXEC | O_APPEND | O_CLOEXEC | O_CREAT
                  | O_DIRECTORY | O_DSYNC | O_EXCL | O_NOCTTY | O_NOFOLLOW | O_NONBLOCK
                  | O_SYNC | O_TRUNC | O_ACCMODE)) {
        errno = EINVAL;
        return -1;
    }

    if (strlen(filename) >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return -1;
    }

    int new_fd = zagtos_allocate_file_descriptor();
    if (new_fd < 0) {
        return -1;
    }

    ZDirectory *dir = NULL;
    ZObject *obj = NULL;
    ZObject *creation_dir = NULL;

    if (dir_fd != AT_FDCWD && filename[0] != '/') {
        ZFileDescriptor *zfd = zagtos_get_file_descriptor_object(dir_fd);
        if (!zfd) {
            goto fail;
        }
        if (!zagtos_compare_uuid(zfd->object->info.type, TYPE_DIRECTORY)) {
            errno = ENOTDIR;
            goto fail;
        }
        dir = (ZDirectory *)zfd->object;
    }

    if (flags & O_CLOEXEC) {
        zagtos_set_file_descriptor_cloexec(new_fd, 1);
    }

    obj = zagtos_get_object_by_path(filename, dir);
    if (obj) {
        if (flags & O_EXCL) {
            errno = EEXIST;
            goto fail;
        }

        _Bool is_dir = zagtos_compare_uuid(obj->info.type, TYPE_DIRECTORY);
        if (!is_dir) {
            if (flags & O_DIRECTORY) {
                errno = ENOTDIR;
                goto fail;
            }
            if (flags & O_EXEC) {
                /* we don't support opening files for executing */
                errno = EACCES;
                goto fail;
            }
        }

        if ((flags & (O_WRONLY | O_RDWR)) && !zagtos_is_object_writeable(obj)) {
            errno = EACCES;
            goto fail;
        }
    } else {
        /* file does not exist */
        if (!(flags & O_CREAT)) {
            errno = ENOENT;
            goto fail;
        }

        char filename_to_modify[PATH_MAX];
        strcpy(filename_to_modify, filename);
        char *name = basename(filename_to_modify);
        if (strlen(name) > 256) {
            errno = ENAMETOOLONG;
            goto fail;
        }
        ZObject *creation_dir = zagtos_get_object_by_path(dirname(filename_to_modify), dir);
        if (!creation_dir) {
            errno = ENOENT;
            goto fail;
        }
        if (!zagtos_compare_uuid(creation_dir->info.type, TYPE_DIRECTORY)) {
            errno = ENOTDIR;
            goto fail;
        }

        obj = zagtos_create_object(TYPE_UNSPECIFIED, 0);
        if (!obj) {
            errno = ENOSPC;
            goto fail;
        }
        if (!zagtos_add_object_to_directory(&obj->info, (ZDirectory *)creation_dir, name)) {
            errno = ENOSPC;
            goto fail;
        }
        errno = ENOSYS;
        goto fail;
    }

    ZFileDescriptor *new_zfd = zagtos_get_file_descriptor_object(new_fd);
    new_zfd->object = obj;
    new_zfd->flags = flags;

    if (flags | O_WRONLY) {
        new_zfd->write = 1;
    }
    if (flags | O_RDWR) {
        new_zfd->read = 1;
        new_zfd->write = 1;
    }

    return new_fd;

fail:
    if (creation_dir) {
        zagtos_put_object(creation_dir);
    }
    if (obj) {
        zagtos_put_object(obj);
    }
    zagtos_free_file_descriptor(new_fd);
    return -1;
}
