#include <errno.h>
#include <malloc.h>
#include <reent.h>

#include <muteki/errno.h>
#include <muteki/fs.h>

#include "mutekishims_utils.h"
#include "nowide.h"

// Might be used by mktemp, etc.
int _mkdir_r(struct _reent *r, const char *path, int mode) {
    UTF16 *wpath = __nowide_prep_path_for_syscall_r(r, path);
    if (!wpath) {
        return -1;
    }

    if (_wmkdir(wpath) < 0) {
        free(wpath);
        _REENT_ERRNO(r) = __muteki_kerrno_to_errno(_GetLastError());
        return -1;
    }

    free(wpath);
    _REENT_ERRNO(r) = 0;
    return 0;
}

int mkdir(const char *path, int mode) {
    return _mkdir_r(_REENT, path, mode);
}
