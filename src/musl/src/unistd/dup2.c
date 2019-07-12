#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <zagtos/unixcompat.h>
#include "syscall.h"

int dup2(int old, int new)
{
    if (!zagtos_get_file_descriptor_object(old)) {
        errno = EBADF;
        return -1;
    }

    if (old!=new) {
        zagtos_clone_file_descriptor(old, new, 1);
	}
    return new;
}
