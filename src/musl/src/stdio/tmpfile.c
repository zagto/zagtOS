#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "stdio_impl.h"

#define MAXTRIES 100

FILE *tmpfile(void)
{
	char s[] = "/tmp/tmpfile_XXXXXX";
	int fd;
	FILE *f;
	int try;
	for (try=0; try<MAXTRIES; try++) {
		__randname(s+13);
        fd = open(s, O_RDWR|O_CREAT|O_EXCL, 0600);
		if (fd >= 0) {
            unlink(s);
			f = __fdopen(fd, "w+");
            if (!f) close(fd);
			return f;
		}
	}
	return 0;
}

weak_alias(tmpfile, tmpfile64);
