#include <errno.h>
#include <muteki/errno.h>

int __muteki_besta_errno_to_errno(muteki_errno_t besta_errno) {
    switch (besta_errno) {
        case FS_INVALID_DRIVE_LETTER:
            return ENODEV;
        case FS_INVALID_FILENAME:
            return EINVAL;
        case FS_OPERATION_ERROR:
            return EIO;
        case FS_ENTRY_EXISTS:
            return EEXIST;
        case FS_FILE_UNAVAILABLE:
        case FS_NO_SUCH_ENTRY:
        case FS_NO_SUCH_ENTRY_ALT:
            return ENOENT;
        case FS_NO_SPACE_LEFT:
            return ENOSPC;
        case MEDIUM_WP_ENABLED:
            return EROFS;
        case FS_FILE_LOCKED:
            return EBUSY;
        default:
            return 0;
    }
}
