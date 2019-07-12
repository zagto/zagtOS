#include <fcntl.h>
#include <stdarg.h>
#include "__open_common.h"

int openat(int dir_fd, const char *filename, int flags, ...)
{
	mode_t mode = 0;

	if ((flags & O_CREAT) || (flags & O_TMPFILE) == O_TMPFILE) {
		va_list ap;
		va_start(ap, flags);
		mode = va_arg(ap, mode_t);
		va_end(ap);
	}

    return __open_common(dir_fd, filename, flags, mode);
}

weak_alias(openat, openat64);
