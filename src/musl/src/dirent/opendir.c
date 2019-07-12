#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "__dirent.h"
#include <zagtos/filesystem.h>
#include <zagtos/object.h>

DIR *opendir(const char *name)
{
    ZObject *obj = zagtos_get_object_by_path(name, NULL);
    if (!obj) {
        errno = ENOENT;
        return NULL;
    }

    if (!zagtos_compare_uuid(obj->info.type, TYPE_DIRECTORY)) {
        errno = ENOTDIR;
        zagtos_put_object(obj);
        return NULL;
    }

    ZDirectory *zdir = (ZDirectory *)obj;

    DIR *dir = zagtos_directory_to_dirstream(zdir);
    zagtos_put_object(zdir);
    return dir;
}
