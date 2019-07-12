#include <unistd.h>
#include <errno.h>
#include <zagtos/filesystem.h>
#include "syscall.h"

int chdir(const char *path)
{
    ZObject *obj = zagtos_get_object_by_path(path, NULL);
    if (!obj) {
        errno = ENOENT;
        return -1;
    }
    if (!zagtos_compare_uuid(obj->info.type, TYPE_DIRECTORY)) {
        errno = ENOTDIR;
        zagtos_put_object(obj);
        return -1;
    }
    zagtos_set_current_directory(path, (ZDirectory *)obj);
    return 0;
}
