/**
 * Core syscalls used by newlib.
 */

#include <errno.h>
#include <reent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <muteki/datetime.h>

// _exit() is defined in program_lifecycle.c

// close
int _close_r(struct _reent *r, int fd) {
	r->_errno = ENOSYS;
	return -1;
}

// environ
char *__env[1] = { 0 };
char **environ = __env;

// execve
int _execve_r(struct _reent *r, const char *name, char * const *argv, char * const *env) {
	r->_errno = ENOSYS;
	return -1;
}

// fork
int _fork_r(struct _reent *r) {
    r->_errno = ENOSYS;
    return -1;
}

// fstat
int _fstat_r(struct _reent *r, int fd, struct stat *st) {
    r->_errno = ENOSYS;
    return -1;
}

// getpid
int _getpid_r(struct _reent *r) {
    r->_errno = ENOSYS;
    return -1;
}

// gettimeofday
int _gettimeofday_r(struct _reent *r, struct timeval *tp, struct timezone *tzp) {
    struct tm dt_unix;
    datetime_t dt_besta;

    if (tp == NULL) {
        r->_errno = EFAULT;
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
    r->_errno = ENOSYS;
    return -1;
}

// kill
int _kill_r(struct _reent *r, int pid, int sig) {
    r->_errno = ENOSYS;
    return -1;
}

// link
int _link_r(struct _reent *r, const char *old, const char *new) {
    r->_errno = ENOSYS;
    return -1;
}

// lseek
int _lseek_r(struct _reent *r, int fd, _off_t ptr, int dir) {
    r->_errno = ENOSYS;
    return -1;
}

// open
int _open_r(struct _reent *r, const char *name, int flags, int mode) {
    r->_errno = ENOSYS;
    return -1;
}

// read
_ssize_t _read_r(struct _reent *r, int fd, char *ptr, _ssize_t len) {
    r->_errno = ENOSYS;
    return -1;
}

// sbrk
void *_sbrk_r(struct _reent *r, int incr) {
    // Does not make sense for Besta. Stub it.
    r->_errno = ENOSYS;
    return NULL;
}

// stat
int _stat_r(struct _reent *r, char *name, struct stat *st) {
    r->_errno = ENOSYS;
    return -1;
}

// times
clock_t _times_r(struct _reent *r, struct tms *buf) {
    r->_errno = ENOSYS;
    return 0;
}

// unlink
int _unlink_r(struct _reent *r, char *name) {
    r->_errno = ENOSYS;
    return -1;
}

// wait
int _wait_r(struct _reent *r, int *status) {
    r->_errno = ENOSYS;
    return -1;
}

// write
_ssize_t _write_r(struct _reent *r, int fd, const void *buf, size_t len) {
	r->_errno = ENOSYS;
	return -1;
}

