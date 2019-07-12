#include <unistd.h>
#include <errno.h>
#include <zagtos/program.h>
#include <zagtos/filesystem.h>
#include <zagtos/unixcompat.h>
#include "syscall.h"

int execve(const char *path, char *const argv[], char *const envp[])
{
    ZObjectInfo *obj_info = zagtos_get_object_info_by_path(path, NULL);
    if (!obj_info) {
        errno = ENOENT;
        return  -1;
    }
    if (!zagtos_compare_uuid(obj_info->type, TYPE_PROGRAM)) {
        zagtos_put_object_info(obj_info);
        errno = EACCES;
        return -1;
    }

    ZUnixRun *unix_run = zagtos_create_unix_run(argv, envp);
    zagtos_send_message(obj_info->id, unix_run);
    zagtos_put_object(unix_run);
    zagtos_put_object_info(obj_info);
    return -1;
}
