#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <zagtos/filesystem.h>
#include <zagtos/unixcompat.h>
#include "__dirent.h"

DIR *fdopendir(int fd)
{
    if (fd >= 0 && fd < 3) { /* stdio */
        errno = ENOTDIR;
        return 0;
    }
    ZFileDescriptor *zfd = zagtos_get_file_descriptor_object(fd);
    if (!zfd || !zfd->read) {
        errno = EBADF;
        return 0;
    }
    if (!zagtos_compare_uuid(zfd->object->info.type, TYPE_DIRECTORY)) {
        errno = ENOTDIR;
        return 0;
    }

    ZDirectory *zdir = (ZDirectory *)zfd->object;

    DIR *dir = zagtos_directory_to_dirstream(zdir);
    if (dir) {
        zagtos_free_file_descriptor(fd);
    }
	return dir;
}
