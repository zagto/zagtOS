#include <dirent.h>
#include <unistd.h>
#include "__dirent.h"
#include "lock.h"

void seekdir(DIR *dir, long off)
{
	LOCK(dir->lock);
    dir->current_entry = off;
    UNLOCK(dir->lock);
}
