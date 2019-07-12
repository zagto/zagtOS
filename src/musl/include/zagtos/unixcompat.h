#ifndef __ZAGTOS_UNIXCOMPAT_H
#define __ZAGTOS_UNIXCOMPAT_H

#include <zagtos/uuid.h>
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

static const ZUUID TYPE_UNIX_RUN = {0x63e228967d0349bf, 0xb1f5dbd07ccfba56};

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
