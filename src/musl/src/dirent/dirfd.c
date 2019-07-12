#include <dirent.h>
#include <errno.h>

int dirfd(DIR *d)
{
    errno = ENOTSUP;
    return -1;
}
