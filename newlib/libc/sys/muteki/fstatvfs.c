#include <errno.h>
#include <malloc.h>
#include <reent.h>
#include <string.h>
#include <unistd.h>

#include <sys/statvfs.h>

#include <muteki/errno.h>
#include <muteki/fs.h>

#include "bestadescriptor.h"
#include "mutekishims_utils.h"
#include "nowide.h"
#include "statvfs_common.h"

int fstatvfs(int __fd, struct statvfs *__buf) {
    int ret;
    DescriptorTranslation *fdmap = __muteki_fd_grab(__fd);

    if (!fdmap) {
        errno = EBADF;
        return -1;
    }

    switch (fdmap->type) {
    case MUTEKI_DESCRIPTOR_FILE:
    case MUTEKI_DESCRIPTOR_DIRECTORY: {
        __nowide_mbstate_t ctx = {0};

        char *rpath = malloc(SYS_PATH_MAX_CU * 3);
        if (rpath == NULL) {
            errno = ENOMEM;
            return -1;
        }

        size_t ret = __nowide_bestawcstombs_r(_REENT, rpath, fdmap->filename, SYS_PATH_MAX_CU * 3, &ctx);
        if (ret == ((size_t) -1)) {
            free(rpath);
            __muteki_fd_drop(fdmap);
            return -1;
        }

        int statvfs_ret = __statvfs_drive(rpath[0], __buf);
        free(rpath);
        __muteki_fd_drop(fdmap);
        return statvfs_ret;
    }
    case MUTEKI_DESCRIPTOR_CHARDEV: {
        memset(__buf, 0, sizeof(*__buf));
        __buf->f_bsize = 512;
        __buf->f_frsize = 512;
        __muteki_fd_drop(fdmap);
        return 0;
    }
    default: {
        __muteki_fd_drop(fdmap);
        errno = EBADF;
        return -1;
    }
    }
}
