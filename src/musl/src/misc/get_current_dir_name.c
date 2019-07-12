#define _GNU_SOURCE
#include <unistd.h>
#include <zagtos/filesystem.h>

char *get_current_dir_name(void) {
    return zagtos_get_current_directory_name();
}
