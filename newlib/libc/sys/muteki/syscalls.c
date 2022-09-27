/**
 * Core syscalls used by newlib.
 */

#include <errno.h>
#include <fcntl.h>
#include <reent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

static int whence_to_besta(int whence, int *err) {
    switch (whence) {
    case SEEK_SET:
        return _SYS_SEEK_SET;
    case SEEK_CUR:
        return _SYS_SEEK_CUR;
    case SEEK_END:
        return _SYS_SEEK_END;
    default:
        if (err != NULL) {
            *err = EINVAL;
        }
        return 0;
    }
}

static time_t find_timestamp_to_unix(unsigned int find_ts) {
    struct tm dt;

    dt.tm_year = FIND_TS_YEAR(find_ts) - 1900;
    dt.tm_mon = FIND_TS_MONTH(find_ts) - 1;
    dt.tm_mday = FIND_TS_DAY(find_ts);
    dt.tm_hour = FIND_TS_HOUR(find_ts);
    dt.tm_min = FIND_TS_MINUTE(find_ts);
    dt.tm_sec = FIND_TS_SECOND(find_ts);
    dt.tm_isdst = -1;

    return mktime(&dt);
}

static int stat_from_find_ctx(struct stat *out, find_context_t *in) {
    memset(out, 0, sizeof(struct stat));

    out->st_size = (off_t) (in->size & 0x7fffffffl);
    out->st_atime = find_timestamp_to_unix(in->atime);
    out->st_mtime = find_timestamp_to_unix(in->mtime);
    // Linux maps ctime to FAT btime, but Besta's btime is not usable due to a bug (?) on the OS side. So we map ctime to mtime instead.
    out->st_ctime = out->st_mtime;

    if (in->attrib & ATTR_DIR) {
        out->st_mode |= _IFDIR;
    } else {
        out->st_mode |= _IFREG;
    }

    if ((sizeof(off_t) == 4) && (in->size & 0x80000000ul)) {
        return -EOVERFLOW;
    }
    return 0;
}

static find_context_t *__muteki_wfind_under_dir_r(struct _reent *r, const UTF16 *wname) {
    // Process wname into a fnmatch pattern
    UTF16 *fnmatch = __muteki_dir_to_fnmatch(wname);
    if (fnmatch == NULL) {
        _REENT_ERRNO(r) = EINVAL;
        return NULL;
    }

    // Create a new find context and use it as the system directory handle
    find_context_t *find_ctx = malloc(sizeof(find_context_t));
    if (find_ctx == NULL) {
        _REENT_ERRNO(r) = ENOMEM;
        free(fnmatch);
        return NULL;
    }

    if (_wfindfirst(fnmatch, find_ctx, 0) < 0) {
        _REENT_ERRNO(r) = __muteki_kerrno_to_errno(_GetLastError());
        free(fnmatch);
        free(find_ctx);
        return NULL;
    }

    free(fnmatch);
    return find_ctx;
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
    find_context_t find_ctx;
    DescriptorTranslation *dt = __muteki_fd_grab(fd);

    if (_wfindfirst(dt->filename, &find_ctx, 0) < 0) {
        _REENT_ERRNO(r) = __muteki_kerrno_to_errno(_GetLastError());
        __muteki_fd_drop(dt);
        return -1;
    }

    int result = stat_from_find_ctx(st, &find_ctx);
    _findclose(&find_ctx);
    if (result < 0) {
        _REENT_ERRNO(r) = -result;
        __muteki_fd_drop(dt);
        return -1;
    }

    __muteki_fd_drop(dt);
    return 0;
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
    // TODO: un-hardcode this after custom std* is possible
    if (fd < 3) {
        return 1;
    }

    if (!is_fd_valid(fd)) {
        _REENT_ERRNO(r) = EBADF;
        return 0;
    }

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
_off_t _lseek_r(struct _reent *r, int fd, _off_t offset, int whence) {
    DescriptorTranslation *dt = __muteki_fd_grab(fd);
    if (dt == NULL) {
        _REENT_ERRNO(r) = EBADF;
        return -1;
    }

    switch (dt->type) {
    case MUTEKI_DESCRIPTOR_DEVNULL: {
        __muteki_fd_drop(dt);
        return 0;
    }
    case MUTEKI_DESCRIPTOR_FILE: {
        // TODO check off_t sizes
        int whence_err = 0;
        int sys_whence = whence_to_besta(whence, &whence_err);
        if (whence_err != 0) {
            _REENT_ERRNO(r) = whence_err;
            __muteki_fd_drop(dt);
            return -1;
        }
        int result = __fseek(dt->handle, offset, sys_whence);
        if (result < 0) {
            _REENT_ERRNO(r) = __muteki_kerrno_to_errno(_GetLastError());
            __muteki_fd_drop(dt);
            return -1;
        }
        _off_t current_pos = _ftell(dt->handle);
        __muteki_fd_drop(dt);
        return current_pos;
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

// open
int _open_r(struct _reent *r, const char *name, int flags, int mode) {
    void *sys_fd = NULL;
    bool exists, is_dir;

    if (strlen(name) == 0) {
        _REENT_ERRNO(r) = ENOENT;
        return -1;
    }

    UTF16 *wname = __nowide_prep_path_for_syscall_r(r, name);
    if (wname == NULL) {
        return -1;
    }

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
        if (!(flags & O_DIRECTORY)) {
            _REENT_ERRNO(r) = EISDIR;
            free(wname);
            return -1;
        }

        sys_fd = __muteki_wfind_under_dir_r(r, wname);

        if (sys_fd == NULL) {
            free(wname);
            return -1;
        }
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
        __muteki_fd_drop(dt);
        return len;
    }
    case MUTEKI_DESCRIPTOR_FILE: {
        size_t actual = _fread(buf, 1, len, dt->handle);
        if (actual == 0) {
            int errno_converted = __muteki_kerrno_to_errno(_GetLastError());
            __muteki_fd_drop(dt);
            if (errno_converted != 0) {
                _REENT_ERRNO(r) = errno_converted;
                return -1;
            }
            return 0;
        }
        __muteki_fd_drop(dt);
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
    find_context_t find_ctx;

    UTF16 *wname = __nowide_prep_path_for_syscall_r(r, name);
    if (wname == NULL) {
        return -1;
    }

    if (_wfindfirst(wname, &find_ctx, 0) < 0) {
        free(wname);
        _REENT_ERRNO(r) = __muteki_kerrno_to_errno(_GetLastError());
        return -1;
    }
    free(wname);

    int result = stat_from_find_ctx(st, &find_ctx);
    _findclose(&find_ctx);
    if (result < 0) {
        free(wname);
        _REENT_ERRNO(r) = -result;
        return -1;
    }

    return 0;
}

// times
clock_t _times_r(struct _reent *r, struct tms *buf) {
    _REENT_ERRNO(r) = ENOSYS;
    return 0;
}

// unlink
int _unlink_r(struct _reent *r, const char *name) {
    if (strlen(name) == 0) {
        _REENT_ERRNO(r) = ENOENT;
        return -1;
    }

    UTF16 *wname = __nowide_prep_path_for_syscall_r(r, name);
    if (wname == NULL) {
        return -1;
    }

    short attr = _wfgetattr(wname);
    if ((attr & ATTR_DIR) != 0) {
        free(wname);
        _REENT_ERRNO(r) = EISDIR;
        return -1;
    }

    bool result = __wremove(wname);
    free(wname);

    if (!result) {
        _REENT_ERRNO(r) = __muteki_kerrno_to_errno(_GetLastError());
        return -1;
    }
    return 0;
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
        __muteki_fd_drop(dt);
        return len;
    }
    case MUTEKI_DESCRIPTOR_FILE: {
        size_t actual = _fwrite(buf, 1, len, dt->handle);
        if (actual == 0) {
            int errno_converted = __muteki_kerrno_to_errno(_GetLastError());
            __muteki_fd_drop(dt);
            if (errno_converted != 0) {
                _REENT_ERRNO(r) = errno_converted;
                return -1;
            }
            return 0;
        }
        __muteki_fd_drop(dt);
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

