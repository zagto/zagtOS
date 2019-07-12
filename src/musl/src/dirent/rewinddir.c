#include <dirent.h>
#include <unistd.h>
#include "__dirent.h"
#include "lock.h"

void rewinddir(DIR *dir)
{
	LOCK(dir->lock);
    dir->current_entry = 0;
	UNLOCK(dir->lock);
}
