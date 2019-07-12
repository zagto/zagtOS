#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <zagtos/filesystem.h>
#include <zagtos/unixcompat.h>
#include "syscall.h"
#include "pthread_impl.h"

int faccessat(int dir_fd, const char *filename, int amode, int flag)
{
    /* allow the flag, but it is ignored as we have no symlinks */
    if ((amode & ~(R_OK|W_OK|X_OK)) && amode != F_OK) {
        errno = EINVAL;
        return -1;
    }

    struct stat st;
    if (fstatat(dir_fd, filename, &st, 0)) {
        return -1;
    }

    if (((amode & R_OK) && !(st.st_mode & 0400))
            || ((amode & W_OK) && !(st.st_mode & 0200))
            || ((amode & X_OK) && !(st.st_mode & 0100))) {
        errno = EACCES;
        return -1;
    }
    return 0;
}
