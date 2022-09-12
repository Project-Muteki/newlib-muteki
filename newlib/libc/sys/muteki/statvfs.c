#include <errno.h>
#include <malloc.h>
#include <reent.h>
#include <string.h>
#include <unistd.h>

#include <sys/statvfs.h>

#include <muteki/errno.h>
#include <muteki/fs.h>

#include "mutekishims_utils.h"
#include "nowide.h"
#include "statvfs_common.h"

int statvfs(const char *__path, struct statvfs *__buf) {
    char *rpath = __realpath(__path);
    if (rpath == NULL) {
        errno = EFAULT;
        return -1;
    }
    // TODO check for existence?
    int drive_sep = __get_drive(rpath);
    if (drive_sep == 2) {
        int ret = __statvfs_drive(rpath[0], __buf);
        free(rpath);
        return ret;
    // Devices
    } else if (drive_sep == 3) {
        free(rpath);
        memset(__buf, 0, sizeof(*__buf));
        __buf->f_bsize = 512;
        __buf->f_frsize = 512;
        return 0;
    } else {
        free(rpath);
        errno = ENOENT;
        return -1;
    }
}
