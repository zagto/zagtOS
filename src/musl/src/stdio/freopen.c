#include "stdio_impl.h"
#include <fcntl.h>
#include <unistd.h>
#include <zagtos/unixcompat.h>

/* The basic idea of this implementation is to open a new FILE,
 * hack the necessary parts of the new FILE into the old one, then
 * close the new FILE. */

/* Locking IS necessary because another thread may provably hold the
 * lock, via flockfile or otherwise, when freopen is called, and in that
 * case, freopen cannot act until the lock is released. */

FILE *freopen(const char *restrict filename, const char *restrict mode, FILE *restrict f)
{
	int fl = __fmodeflags(mode);
	FILE *f2;

	FLOCK(f);

	fflush(f);

	if (!filename) {
		if (fl&O_CLOEXEC)
            zagtos_set_file_descriptor_cloexec(f->fd, 1);
		fl &= ~(O_CREAT|O_EXCL|O_CLOEXEC);
        ZFileDescriptor *zfd = zagtos_get_file_descriptor_object(f->fd);
        zfd->flags = fl;
        if (fl | O_WRONLY) {
            zfd->write = 1;
            zfd->read = 0;
        } else  if (fl | O_RDWR) {
            zfd->read = 1;
            zfd->write = 1;
        } else {
            zfd->write = 0;
            zfd->read = 1;
        }
	} else {
		f2 = fopen(filename, mode);
		if (!f2) goto fail;
		if (f2->fd == f->fd) f2->fd = -1; /* avoid closing in fclose */
		else if (__dup3(f2->fd, f->fd, fl&O_CLOEXEC)<0) goto fail2;

		f->flags = (f->flags & F_PERM) | f2->flags;
		f->read = f2->read;
		f->write = f2->write;
		f->seek = f2->seek;
		f->close = f2->close;

		fclose(f2);
	}

	FUNLOCK(f);
	return f;

fail2:
	fclose(f2);
fail:
	fclose(f);
	return NULL;
}

weak_alias(freopen, freopen64);