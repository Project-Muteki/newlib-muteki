#include <errno.h>
#include <malloc.h>
#include <reent.h>
#include <unistd.h>

#include <muteki/errno.h>
#include <muteki/fs.h>

#include "mutekishims_utils.h"
#include "nowide.h"

int _rmdir_r(struct _reent *r, const char *path) {
    UTF16 *wpath = __nowide_prep_path_for_syscall_r(r, path);
    if (!wpath) {
        return -1;
    }

    if (_wrmdir(wpath) < 0) {
        free(wpath);
        _REENT_ERRNO(r) = __muteki_kerrno_to_errno(_GetLastError());
        return -1;
    }

    free(wpath);
    _REENT_ERRNO(r) = 0;
    return 0;
}

int rmdir(const char *path) {
    return _rmdir_r(_REENT, path);
}
