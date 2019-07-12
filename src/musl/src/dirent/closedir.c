#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include "__dirent.h"

int closedir(DIR *dir)
{
    free(dir->entries);
	free(dir);
    return 0;
}
