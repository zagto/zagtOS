#include "stdio_impl.h"
#include <unistd.h>
#include <sys/uio.h>

size_t __stdio_write(FILE *f, const unsigned char *buf, size_t len)
{
    ssize_t cnt;
    cnt = write(f->fd, buf, len);
    if (cnt == len) {
        f->wend = f->buf + f->buf_size;
        f->wpos = f->wbase = f->buf;
        return len;
    } else {
        f->wpos = f->wbase = f->wend = 0;
        f->flags |= F_ERR;
        return 0;
    }
}
