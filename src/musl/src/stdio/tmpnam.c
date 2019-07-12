#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include "syscall.h"

#define MAXTRIES 100

char *tmpnam(char *buf)
{
	static char internal[L_tmpnam];
	char s[] = "/tmp/tmpnam_XXXXXX";
	int try;
	int r;
	for (try=0; try<MAXTRIES; try++) {
		__randname(s+12);
        r = fstatat(AT_FDCWD, s,
			&(struct stat){0}, AT_SYMLINK_NOFOLLOW);
        if (r == -1) return strcpy(buf ? buf : internal, s);
	}
	return 0;
}
