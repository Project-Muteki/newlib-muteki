/**
 * Core syscalls used by newlib.
 */

#include <errno.h>
#include <fcntl.h>
#include <reent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "bestadescriptor.h"
#include "nowide.h"
#include "mutekishims_utils.h"

#include <muteki/common.h>
#include <muteki/datetime.h>
#include <muteki/errno.h>
#include <muteki/file.h>
#include <muteki/fs.h>

const UTF16 __READ_BINARY[] = _BUL("rb");
const UTF16 __RW_BINARY[] = _BUL("rb+");
const UTF16 __RW_BINARY_TRUNC[] = _BUL("wb+");
const UTF16 __APPEND_BINARY[] = _BUL("ab+");

extern int mutekix_console_printf(const char *fmt, ...);

static const UTF16 *open_flag_to_besta(int flags) {
    int accmode = flags & O_ACCMODE;
    switch (accmode) {
    case O_WRONLY:
    case O_RDWR:
        if (flags & O_APPEND) {
            return __APPEND_BINARY;
        }
        if (flags & O_TRUNC) {
            return __RW_BINARY_TRUNC;
        }
        return __RW_BINARY;
    case O_RDONLY:
        return __READ_BINARY;
    default:
        return NULL;
    }
}

static UTF16 *dir_to_fnmatch(UTF16 *path) {
    size_t wlen = __nowide_bestawcslen(path);
    if (wlen < 1) {
        return NULL;
    }
    if (path[wlen - 1] == '\\') {
        UTF16 *fnmatch = realloc(path, (wlen + 2) * sizeof(UTF16));
        fnmatch[wlen] = _BUL('*');
        return fnmatch;
    } else {
        UTF16 *fnmatch = realloc(path, (wlen + 3) * sizeof(UTF16));
        fnmatch[wlen] = _BUL('\\');
        fnmatch[wlen+1] = _BUL('*');
        return fnmatch;
    }
}

// _exit() is defined in program_lifecycle.c

// close
int _close_r(struct _reent *r, int fd) {
    if (__muteki_release_descriptor(fd) < 0) {
        _REENT_ERRNO(r) = EBADF;
        return -1;
    }
    return 0;
}

// environ
char *__env[1] = { 0 };
char **environ = __env;

// execve
int _execve_r(struct _reent *r, const char *name, char * const *argv, char * const *env) {
    _REENT_ERRNO(r) = ENOSYS;
    return -1;
}

// fork
int _fork_r(struct _reent *r) {
    _REENT_ERRNO(r) = ENOSYS;
    return -1;
}

// fstat
int _fstat_r(struct _reent *r, int fd, struct stat *st) {
    _REENT_ERRNO(r) = ENOSYS;
    return -1;
}

// getpid
int _getpid_r(struct _reent *r) {
    _REENT_ERRNO(r) = ENOSYS;
    return -1;
}

// gettimeofday
int _gettimeofday_r(struct _reent *r, struct timeval *tp, void *tzp) {
    struct tm dt_unix;
    datetime_t dt_besta;

    if (tp == NULL) {
        _REENT_ERRNO(r) = EFAULT;
        return -1;
    }

    GetSysTime(&dt_besta);

    dt_unix.tm_year = dt_besta.year - 1900;
    dt_unix.tm_mon = dt_besta.month - 1;
    dt_unix.tm_mday = dt_besta.day;
    dt_unix.tm_hour = dt_besta.hour;
    dt_unix.tm_min = dt_besta.minute;
    dt_unix.tm_sec = dt_besta.second;
    dt_unix.tm_isdst = -1;

    tp->tv_sec = mktime(&dt_unix);
    tp->tv_usec = dt_besta.millis * 1000;

    return 0;
}

// isatty
int _isatty_r(struct _reent *r, int fd) {
    // std* is definitely TTY.
    if (fd < 3) {
        return 1;
    }
    // TODO query the FD and set errno to EBADF if descriptor is not used.
    _REENT_ERRNO(r) = ENOTTY;
    return 0;
}

// kill
int _kill_r(struct _reent *r, int pid, int sig) {
    _REENT_ERRNO(r) = ENOSYS;
    return -1;
}

// link
int _link_r(struct _reent *r, const char *old, const char *new) {
    _REENT_ERRNO(r) = ENOSYS;
    return -1;
}

// lseek
_off_t _lseek_r(struct _reent *r, int fd, _off_t ptr, int dir) {
    _REENT_ERRNO(r) = ENOSYS;
    return -1;
}

// open
int _open_r(struct _reent *r, const char *name, int flags, int mode) {
    void *sys_fd = NULL;
    bool exists, is_dir;

    if (strlen(name) == 0) {
        _REENT_ERRNO(r) = ENOENT;
        return -1;
    }

    UTF16 *wname = __nowide_path_a2w_r(r, name);
    if (wname == NULL) {
        return -1;
    }

    mutekix_console_printf("flags: 0x%x\n", flags);

    short attr = _wfgetattr(wname);

    if (attr < 0) {
        exists = false;
        is_dir = false;
    } else {
        exists = true;
        is_dir = (attr & ATTR_DIR) != 0;
    }

    if (!(flags & O_CREAT) && !exists) {
        _REENT_ERRNO(r) = ENOENT;
        free(wname);
        return -1;
    }

    if (is_dir) {
        // Process wname into a fnmatch pattern
        if (flags & O_DIRECTORY) {
            _REENT_ERRNO(r) = EISDIR;
            free(wname);
            return -1;
        }

        UTF16 *fnmatch = dir_to_fnmatch(wname);

        if (fnmatch == NULL) {
            _REENT_ERRNO(r) = EINVAL;
            free(wname);
            return -1;
        }

        // Create a new find context and use it as the system directory handle
        find_context_t *find_ctx = malloc(sizeof(find_context_t));
        if (_wfindfirst(fnmatch, find_ctx) < 0) {
            _REENT_ERRNO(r) = __muteki_kerrno_to_errno(_GetLastError());
            free(wname);
            free(fnmatch);
            free(find_ctx);
            return -1;
        }

        sys_fd = find_ctx;
        free(fnmatch);
    } else {
        if (flags & O_DIRECTORY) {
            _REENT_ERRNO(r) = ENOTDIR;
            free(wname);
            return -1;
        }

        const UTF16 *besta_mode = open_flag_to_besta(flags);
        if (besta_mode == NULL) {
            _REENT_ERRNO(r) = EINVAL;
            free(wname);
            return -1;
        }

        sys_fd = __wfopen(wname, besta_mode);
        if (sys_fd == NULL) {
            _REENT_ERRNO(r) = __muteki_kerrno_to_errno(_GetLastError());
            free(wname);
            return -1;
        }
    }

    // Create the descriptor and keep the path string alive
    int fd = __muteki_acquire_descriptor();
    DescriptorTranslation *dt = __muteki_fd_grab(fd);
    dt->handle = sys_fd;
    dt->type = is_dir ? MUTEKI_DESCRIPTOR_DIRECTORY : MUTEKI_DESCRIPTOR_FILE;
    dt->filename = wname;
    __muteki_fd_drop(dt);
    return fd;
}

// read
_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t len) {
    DescriptorTranslation *dt = __muteki_fd_grab(fd);
    if (dt == NULL) {
        _REENT_ERRNO(r) = EBADF;
        return -1;
    }

    switch (dt->type) {
    case MUTEKI_DESCRIPTOR_DEVNULL: {
        memset(buf, 0, len);
        return len;
    }
    case MUTEKI_DESCRIPTOR_FILE: {
        size_t actual = _fread(buf, 1, len, dt->handle);
        __muteki_fd_drop(dt);
        if (actual == 0) {
            int errno_converted = __muteki_kerrno_to_errno(_GetLastError());
            if (errno_converted != 0) {
                _REENT_ERRNO(r) = errno_converted;
                return -1;
            }
            return 0;
        }
        return (_ssize_t) (actual & 0x7fffffff);
    }
    case MUTEKI_DESCRIPTOR_DIRECTORY: {
        __muteki_fd_drop(dt);
        _REENT_ERRNO(r) = EISDIR;
        return -1;
    }
    case MUTEKI_DESCRIPTOR_CHARDEV: // TODO
    default: {
        __muteki_fd_drop(dt);
        _REENT_ERRNO(r) = ENOSYS;
        return -1;
    }
    }
}

// sbrk
void *_sbrk_r(struct _reent *r, int incr) {
    // Does not make sense for Besta. Stub it.
    _REENT_ERRNO(r) = ENOSYS;
    return NULL;
}

// stat
int _stat_r(struct _reent *r, const char *name, struct stat *st) {
    _REENT_ERRNO(r) = ENOSYS;
    return -1;
}

// times
clock_t _times_r(struct _reent *r, struct tms *buf) {
    _REENT_ERRNO(r) = ENOSYS;
    return 0;
}

// unlink
int _unlink_r(struct _reent *r, const char *name) {
    _REENT_ERRNO(r) = ENOSYS;
    return -1;
}

// wait
int _wait_r(struct _reent *r, int *status) {
    _REENT_ERRNO(r) = ENOSYS;
    return -1;
}

// write
_ssize_t _write_r(struct _reent *r, int fd, const void *buf, size_t len) {
    DescriptorTranslation *dt = __muteki_fd_grab(fd);
    if (dt == NULL) {
        _REENT_ERRNO(r) = EBADF;
        return -1;
    }

    switch (dt->type) {
    case MUTEKI_DESCRIPTOR_DEVNULL: {
        return len;
    }
    case MUTEKI_DESCRIPTOR_FILE: {
        size_t actual = _fwrite(buf, 1, len, dt->handle);
        __muteki_fd_drop(dt);
        if (actual == 0) {
            int errno_converted = __muteki_kerrno_to_errno(_GetLastError());
            if (errno_converted != 0) {
                _REENT_ERRNO(r) = errno_converted;
                return -1;
            }
            return 0;
        }
        return (_ssize_t) (actual & 0x7fffffff);
    }
    case MUTEKI_DESCRIPTOR_DIRECTORY: {
        __muteki_fd_drop(dt);
        _REENT_ERRNO(r) = EISDIR;
        return -1;
    }
    case MUTEKI_DESCRIPTOR_CHARDEV: // TODO
    default: {
        __muteki_fd_drop(dt);
        _REENT_ERRNO(r) = ENOSYS;
        return -1;
    }
    }
}

