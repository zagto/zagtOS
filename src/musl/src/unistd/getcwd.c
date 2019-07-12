#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <zagtos/filesystem.h>
#include <string.h>
#include "syscall.h"

char *getcwd(char *buf, size_t size)
{
    if (size == 0 && buf) {
        errno = EINVAL;
        return NULL;
    }

    char *result = zagtos_get_current_directory_name();
    if (!buf || !result) {
        return result;
    } else {
        if (strlen(result) + 1 > size) {
            free(result);
            errno = ERANGE;
            return NULL;
        }

        strcpy(buf, result);
        return buf;
    }
}
