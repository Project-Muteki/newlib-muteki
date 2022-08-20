#include <errno.h>
#include <muteki/errno.h>

int __muteki_kerrno_to_errno(kerrno_t kerrno) {
    short err = KERRNO_ERR(kerrno);
    short ns = KERRNO_NS(kerrno);
    switch (ns) {
    case ERRNO_NS_KERNEL:
        switch (err) {
        case FS_INVALID_DRIVE_LETTER:
            return ENODEV;
        case FS_INVALID_FILENAME:
        case FS_CONFLICTING_ATTR:
        case FTL_INVALID_LBA:
            return EINVAL;
        case FTL_DATA_CORRUPTED:
        case FTL_ECC_FAILED:
        case FS_OPERATION_ERROR:
            return EIO;
        case FS_ENTRY_EXISTS:
            return EEXIST;
        case FS_FILE_UNAVAILABLE:
        case FS_NO_SUCH_ENTRY:
        case FS_NO_SUCH_ENTRY_ALT:
            return ENOENT;
        case FS_DIR_FULL:
        case FS_DIR_FULL_ALT1:
        case FS_DIR_FULL_ALT2:
        case FS_NO_SPACE_LEFT:
            return ENOSPC;
        case MEDIUM_WP_ENABLED:
        case FS_READ_ONLY_FILE:
            return EROFS;
        case FS_FILE_LOCKED:
            return EBUSY;
        case FS_PATH_TOO_LONG:
            return ENAMETOOLONG;
        case FS_TOO_MANY_OPEN_FILES:
            return ENFILE;
        case MEDIUM_UNLOADED:
            return ENOMEDIUM;
        case MEDIUM_INCOMPATIBLE:
            return EMEDIUMTYPE;
        // Explicitly ignore.
        case FS_FILE_ATTR_ERROR: // May be handled in specific C APIs.
        case FS_FILE_OOB_ACCESS: // Just truncate the data/length. TODO verify the truncate behavior
        default:
            return 0;
        }
        break;
    default:
        return 0;
    }
}
