#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include "__dirent.h"
#include "syscall.h"

struct dirent *readdir(DIR *dir)
{
    if (dir->current_entry >= dir->num_entries) {
        return NULL;
    }
    struct dirent *result = &dir->entries[dir->current_entry];
    dir->current_entry++;
    return result;
}

weak_alias(readdir, readdir64);
