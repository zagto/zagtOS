#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <zagtos/filesystem.h>

char *realpath(const char *restrict filename, char *restrict resolved)
{
    char tmp[PATH_MAX * 2];

    if (!filename) {
        errno = EINVAL;
        return 0;
    }

    if (filename[0] == '/') {
        /* absolute path given */
        tmp[0] = 0;
    } else {
        if (!getcwd(tmp, PATH_MAX)) {
            return 0;
        }
    }

    size_t src, dst;
    dst = strlen(tmp);
    src = 0;

    while (filename[src] != 0) {
        if (dst >= PATH_MAX - 1) {
            errno = ENAMETOOLONG;
            return 0;
        }

        if (filename[src] == '.') {
            if (filename[src+1] == '/' || (filename[src+1] == 0 && src > 0)) {
                /* current dir . - ignore */
                src++;
                continue;
            } else if (filename[src+1] == '.' && (filename[src+2] == '/' || filename[src+2] == 0)) {
                /* parent dir .. - go back in destination array */
                if (dst < 3) {
                    /* not at lease /a/ threre */
                    errno = ENOENT;
                    return 0;
                }
                dst -= 2;
                while (tmp[dst-1] != '/') {
                    if (dst == 0) {
                        errno = ENOENT;
                        return 0;
                    }
                    dst--;
                }

                src += 2;
                continue;
            }
        } else if (filename[src] == '/') {
            /* only add one slash if there are multiple */
            if (dst == 0 || tmp[dst-1] != '/') {
                tmp[dst] = '/';
                dst++;
            }
            src++;
            continue;
        }

        /* copy regular name */
        while (filename[src] != 0 && filename[src] != '/') {
            tmp[dst] = filename[src];
            src++;
            dst++;
        }
    }
    tmp[dst] = 0;

    return resolved ? strcpy(resolved, tmp) : strdup(tmp);
}
