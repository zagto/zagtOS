#define _GNU_SOURCE
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <lock.h>
#include <zagtos/unixcompat.h>
#include <zagtos/syscall.h>

int fcntl(int fd, int cmd, ...)
{
	unsigned long arg;
	va_list ap;
	va_start(ap, cmd);
	arg = va_arg(ap, unsigned long);
	va_end(ap);

    ZFileDescriptor *zfd = zagtos_get_file_descriptor_object(fd);
    if (!zagtos_get_file_descriptor_object(fd)) {
        errno = EBADF;
        return -1;
    }

    switch (cmd) {
    case F_DUPFD:
    case F_DUPFD_CLOEXEC: {
        int new_fd = zagtos_clone_file_descriptor(fd, arg, 0);
        if (new_fd < 0) {
            errno = EMFILE;
            return -1;
        }
        if (cmd == F_DUPFD_CLOEXEC) {
            zagtos_set_file_descriptor_cloexec(new_fd, 1);
        }
        return new_fd;
    }
    case F_GETFD:
        return zagtos_get_file_descriptor_cloexec(fd) ? FD_CLOEXEC : 0;
    case F_SETFD:
        zagtos_set_file_descriptor_cloexec(fd, arg & FD_CLOEXEC);
        return 0;
    default:
        // TODO: support remaining modes
        errno = -ENOSYS;
        return -1;
    }
}
