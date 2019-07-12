#include <zagtos/unixcompat.h>
#include <zagtos/object.h>
#include <stdlib.h>
#include <assert.h>
#include <lock.h>
#include <errno.h>


#define MAX_NUM_FILEDES 128
static ZFileDescriptor *file_descriptors[MAX_NUM_FILEDES] = {NULL};
static _Bool cloexec[MAX_NUM_FILEDES] = {0};

static int fd_lock;

void zagtos_init_file_descriptor(int fd, ZFileDescriptor *zfd) {
    file_descriptors[fd] = zfd;
}

int zagtos_allocate_file_descriptor(void) {
    int fd;
    LOCK(&fd_lock);

    for (fd = 3; fd < MAX_NUM_FILEDES && file_descriptors[fd]; fd++);
    if (fd == MAX_NUM_FILEDES) {
        UNLOCK(&fd_lock);
        errno = EMFILE;
        return -1;
    }
    file_descriptors[fd] = calloc(1, sizeof(ZFileDescriptor));
    if (!file_descriptors[fd]) {
        return -1;
    }
    file_descriptors[fd]->refcount = 1;

    UNLOCK(&fd_lock);
    return fd;
}

static _Bool validate_file_descriptor(int fd) {
    _Bool result = fd >= 0 && fd < MAX_NUM_FILEDES && file_descriptors[fd];
    if (!result) {
        errno = EBADF;
    }
}

int zagtos_clone_file_descriptor(int fd, int min_new, _Bool exact) {
    int fd2;
    LOCK(&fd_lock);

    if (!validate_file_descriptor(fd)) {
        UNLOCK(&fd_lock);
        return -1;
    }

    if (exact) {
        if (file_descriptors[min_new]) {
            // "close" existing fd
            zagtos_put_object(file_descriptors[min_new]->object);
        }
        fd2 = min_new;
    } else {
        for (fd2 = min_new; fd2 < MAX_NUM_FILEDES && file_descriptors[fd2]; fd++);
        if (fd2 == MAX_NUM_FILEDES) {
            UNLOCK(&fd_lock);
            return -1;
        }
    }
    LOCK(&file_descriptors[fd]->lock);
    file_descriptors[fd2] = file_descriptors[fd];
    file_descriptors[fd]->refcount++;
    UNLOCK(&file_descriptors[fd]->lock);

    UNLOCK(&fd_lock);
    return fd;
}

int zagtos_set_file_descriptor_cloexec(int fd, _Bool value) {
    LOCK(&file_descriptors[fd]->lock);
    cloexec[fd] = 1;
    UNLOCK(&file_descriptors[fd]->lock);
}

_Bool zagtos_get_file_descriptor_cloexec(int fd) {
    LOCK(&file_descriptors[fd]->lock);
    _Bool result = cloexec[fd];
    UNLOCK(&file_descriptors[fd]->lock);
    return result;
}

int zagtos_free_file_descriptor(int fd) {
    LOCK(&fd_lock);

    if (!validate_file_descriptor(fd)) {
        UNLOCK(&fd_lock);
        return -1;
    }

    ZFileDescriptor *ptr = file_descriptors[fd];
    ptr->refcount--;
    if (!ptr->refcount) {
        if (ptr->object) {
            zagtos_put_object(ptr->object);
        }
        free(ptr);
    }
    file_descriptors[fd] = NULL;
    cloexec[fd] = 0;

    UNLOCK(&fd_lock);
}

ZFileDescriptor *zagtos_get_file_descriptor_object(int fd) {
    if (!validate_file_descriptor(fd)) {
        return NULL;
    }
    return file_descriptors[fd];
}

ZUnixRun *zagtos_create_unix_run(char *const *argv, char *const *env) {
    size_t num_bytes = 0;
    for (size_t i = 0; argv[i] != NULL; i++) {
// TODO
    }
}
