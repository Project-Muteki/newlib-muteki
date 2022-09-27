#include <errno.h>
#include <unistd.h>
#include "bestadescriptor.h"
#include "mutekishims_utils.h"

#include <muteki/file.h>

int fsync(int fd) {
    DescriptorTranslation *dt = __muteki_fd_grab(fd);
    if (dt == NULL) {
        errno = EBADF;
        return -1;
    }
    switch (dt->type) {
    case MUTEKI_DESCRIPTOR_DEVNULL: {
        __muteki_fd_drop(dt);
        return 0;
    }
    case MUTEKI_DESCRIPTOR_FILE: {
        if (__fflush(dt->handle) != 0) {
            int errno_converted = __muteki_kerrno_to_errno(_GetLastError());
            __muteki_fd_drop(dt);
            if (errno_converted != 0) {
                errno = errno_converted;
                return -1;
            }
            return 0;
        }
        __muteki_fd_drop(dt);
        return 0;
    }
    default: {
        __muteki_fd_drop(dt);
        errno = EBADF;
        return -1;
    }
    }
}

int fdatasync(int fd) {
    return fsync(fd);
}
