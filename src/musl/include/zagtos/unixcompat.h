#ifndef __ZAGTOS_UNIXCOMPAT_H
#define __ZAGTOS_UNIXCOMPAT_H

#include <uuid/uuid.h>
#include <zagtos/object.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    ZObject object;
    int32_t argc;
    uint32_t environ_count;
    char argv_environ_blob;
} ZUnixRun;

typedef struct {
    uint32_t port_handle;

    // TODO: remove this
    ZObject *object;

    uint64_t position;
    _Bool is_syslog;
    _Bool has_position;
    _Bool read;
    _Bool write;
    volatile int lock;
    int refcount;
    int flags;
} ZFileDescriptor;

UUID_DEFINE(ZAGTOS_MSG_UNIX_RUN, 0x63, 0xe2, 0x28, 0x96, 0x7d, 0x03, 0x49, 0xbf,
            0xb1, 0xf5, 0xdb, 0xd0, 0x7c, 0xcf, 0xba, 0x56);

static const ZUUID TYPE_TTY = {0x8e988befa0f34235, 0xb592ed59cb8bc92d};


void zagtos_init_file_descriptor(int fd, ZFileDescriptor *zfd);
int zagtos_allocate_file_descriptor(void);
int zagtos_clone_file_descriptor(int fd, int min_new, _Bool exact);
int zagtos_set_file_descriptor_cloexec(int fd, _Bool value);
_Bool zagtos_get_file_descriptor_cloexec(int fd);
int zagtos_free_file_descriptor(int fd);
int zagtos_invalidate_file_descriptor(int fd);
ZFileDescriptor *zagtos_get_file_descriptor_object(int fd);
ZUnixRun *zagtos_create_unix_run(char *const *argv, char *const *env);

#endif // UNIXCOMPAT_H
