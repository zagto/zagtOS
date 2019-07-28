#define _GNU_SOURCE
#include <unistd.h>
#include <sys/uio.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

ssize_t readv(int fd, const struct iovec *iov, int count)
{
    if (count <= 0 || count > UIO_MAXIOV) {
        errno = EINVAL;
        return -1;
    }

    int i;
    ssize_t sum = 0;
    for (i = 0; i < count; i++) {
        if (SSIZE_MAX - (size_t)sum < iov[i].iov_len) {
            errno = EINVAL;
            return -1;
        }
    }

    sum = 0;
    for (i = 0; i < count; i++) {
        size_t len = iov[i].iov_len;
        ssize_t actual_len = write(fd, iov[i].iov_base, len);
        if (actual_len < 0) {
            return -1;
        }
        sum += actual_len;
        if ((size_t)actual_len != len) {
            return sum;
        }
    }
    return sum;
}
