#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE 1
#include <dirent.h>
#include <assert.h>
#include <zagtos/object.h>
#include <zagtos/filesystem.h>
#include "../dirent/__dirent.h"


struct __dirstream *zagtos_directory_to_dirstream(ZDirectory *x) {
    assert(0);
    /*DIR *dir = calloc(1, sizeof(DIR));
    if (!dir) {
        return 0;
    }

    dir->entries = calloc(zdir->numEntries, sizeof(struct dirent));
    if (!dir->entries) {
        free(dir);
        return 0;
    }

    for (size_t i = 0; i < zdir->numEntries; i++) {
        ZUUID obj_id = zdir->entries[i].objectId;
        ZObjectInfo oi;
        zagtos_read_object_info(obj_id, &oi);
        dir->entries[i] = (struct dirent) {
            .d_ino = 0,
            .d_off = i,
            .d_reclen = sizeof(struct dirent),
            .d_type = zagtos_compare_uuid(oi.type, TYPE_DIRECTORY) ? DT_DIR : DT_REG,
            .d_objid = obj_id
        };
        memcpy(dir->entries[i].d_name, zdir->entries[i].name, 256);
    }
    return dir;*/
}


void zagtos_put_directory(ZDirectory *dir) {
    zagtos_put_object(dir);
    free(dir);
}


ZObject *zagtos_get_object_by_path(const char *path, ZDirectory *startDir) {
    /* TODO: implement */
    assert(0);
}


_Bool zagtos_add_object_to_directory(ZObjectInfo *objInfo, ZDirectory *dir, const char *name) {
    /* TODO: implement */
    assert(0);
}
