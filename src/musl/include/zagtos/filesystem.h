#ifndef __ZAGTOS_FILESYSTEM_H
#define __ZAGTOS_FILESYSTEM_H

#include <zagtos/messaging-legacy.h>
#include <zagtos/object.h>

struct __dirstream;

typedef struct {
    ZUUID objectId;
    char name[256];
} ZDirectoryEntry;

typedef struct {
    ZObject object;

    uint32_t numEntries;
    ZUUID parent;
    ZDirectoryEntry entries[];
} ZDirectory;

ZObject *zagtos_get_object_by_path(const char *path, ZDirectory *startDir);
ZObjectInfo *zagtos_get_object_info_by_path(const char *path, ZDirectory *startDir);
struct __dirstream *zagtos_directory_to_dirstream(ZDirectory *zdir);
_Bool zagtos_add_object_to_directory(ZObjectInfo *objInfo, ZDirectory *dir, const char *name);
char *zagtos_canonicalize_path(const char *orig);
char *zagtos_get_current_directory_name();
void zagtos_set_current_directory(const char *path, ZDirectory *dir);


static const ZUUID TYPE_DIRECTORY = {{0x6860b94fc8bc4528, 0x9bb85f03209a5e7f}};

#endif // FILESYSTEM_H
