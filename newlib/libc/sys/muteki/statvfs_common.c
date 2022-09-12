#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <muteki/errno.h>
#include <muteki/fs.h>

#include "statvfs_common.h"
#include "mutekishims_utils.h"

static int drive_to_fsid_simple(char drive) {
    drive = toupper(drive);

    if (drive < 'A' || drive > 'Z') {
        return -1;
    }

    register int fs_id = drive - 'A';

    if (fs_id <= 2) {
        return (fs_id + 1) % 3;
    }

    return fs_id;
}

int __statvfs_drive(char drive, struct statvfs *buf) {
    fs_stat_t fs_stat = {0};
    int fsid = drive_to_fsid_simple(drive);
    if (fsid < 0) {
        errno = EINVAL;
        return -1;
    }
    int ret = FSGetDiskRoomState(fsid, &fs_stat);
    if (ret < 0) {
        errno = __muteki_kerrno_to_errno(_GetLastError());
        return -1;
    }

    memset(buf, 0, sizeof(*buf));

    buf->f_bsize = 512;
    // There's no known way to get cluster size. Use minimum sector size instead.
    buf->f_frsize = 512;
    buf->f_blocks = fs_stat.size / 512ull;
    buf->f_bfree = fs_stat.free / 512ull;
    buf->f_bavail = buf->f_bfree;
    buf->f_files = (fsfilcnt_t) (-1);
    buf->f_ffree = (fsfilcnt_t) (-1);
    buf->f_favail = (fsfilcnt_t) (-1);
    buf->f_fsid = fsid;
    // TODO Set ST_RDONLY on system or write protected drives.
    //buf->f_flag = 0;
    // TODO This is technically not true. It will be 256 CU but not 256 bytes.
    buf->f_namemax = 256;
    return 0;
}
