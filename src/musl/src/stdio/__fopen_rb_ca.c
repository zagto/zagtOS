#include "stdio_impl.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <zagtos/unixcompat.h>

FILE *__fopen_rb_ca(const char *filename, FILE *f, unsigned char *buf, size_t len)
{
	memset(f, 0, sizeof *f);

    f->fd = open(filename, O_RDONLY|O_CLOEXEC);
	if (f->fd < 0) return 0;

    zagtos_set_file_descriptor_cloexec(f->fd, 1);

	f->flags = F_NOWR | F_PERM;
	f->buf = buf + UNGET;
	f->buf_size = len - UNGET;
	f->read = __stdio_read;
	f->seek = __stdio_seek;
	f->close = __stdio_close;
	f->lock = -1;

	return f;
}
