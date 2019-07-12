#include "stdio_impl.h"
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <zagtos/unixcompat.h>
#include "libc.h"

FILE *__fdopen(int fd, const char *mode)
{
	FILE *f;

	/* Check for valid initial mode character */
	if (!strchr("rwa", *mode)) {
		errno = EINVAL;
		return 0;
	}

    /* Get internal file descriptor object */
    ZFileDescriptor *zfd = zagtos_get_file_descriptor_object(fd);
    if (!zagtos_get_file_descriptor_object(fd)) {
        errno = EBADF;
        return NULL;
    }

	/* Allocate FILE+buffer or fail */
	if (!(f=malloc(sizeof *f + UNGET + BUFSIZ))) return 0;

	/* Zero-fill only the struct, not the buffer */
	memset(f, 0, sizeof *f);

	/* Impose mode restrictions */
	if (!strchr(mode, '+')) f->flags = (*mode == 'r') ? F_NOWR : F_NORD;

	/* Apply close-on-exec flag */
    if (strchr(mode, 'e')) zagtos_set_file_descriptor_cloexec(fd, 1);

	/* Set append mode on fd if opened for append */
	if (*mode == 'a') {
        zfd->flags | O_APPEND;
		f->flags |= F_APP;
	}

	f->fd = fd;
	f->buf = (unsigned char *)f + sizeof *f + UNGET;
	f->buf_size = BUFSIZ;

	/* Initialize op ptrs. No problem if some are unneeded. */
	f->read = __stdio_read;
	f->write = __stdio_write;
	f->seek = __stdio_seek;
	f->close = __stdio_close;

	if (!libc.threaded) f->lock = -1;

	/* Add new FILE to open file list */
	return __ofl_add(f);
}

weak_alias(__fdopen, fdopen);
