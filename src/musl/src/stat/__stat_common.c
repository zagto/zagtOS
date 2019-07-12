#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <zagtos/filesystem.h>
#include <zagtos/program.h>
#include "__stat_common.h"

int __stat_common(const ZObjectInfo *restrict obj_info, struct stat *restrict buf) {
    /* assuming off_t is 64 bits */
    if (obj_info->num_data_bytes > INT64_MAX) {
        errno = EOVERFLOW;
        return -1;
    } else {
        buf->st_size = obj_info->num_data_bytes;
    }

    if (zagtos_compare_uuid(obj_info->type, TYPE_DIRECTORY)) {
        buf->st_mode = 0555;
    } else if (zagtos_compare_uuid(obj_info->type, TYPE_PROGRAM)) {
        buf->st_mode = 0555;
    } else {
        buf->st_mode = 0444;
    }
    if (obj_info->flags & ZOBJECT_WRITEABLE) {
        buf->st_mode |= 0222;
    }

    return 0;
}
