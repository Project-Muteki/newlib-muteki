#include <reent.h>
#include <stdlib.h>

#include <muteki/errno.h>
#include <muteki/fs.h>
#include "nowide.h"
#include "mutekishims_utils.h"

int _rename_r (struct _reent *ptr, const char *old, const char *new) {
    UTF16 *wold = __nowide_prep_path_for_syscall_r(ptr, old);
    if (wold == NULL) {
        return -1;
    }
    UTF16 *wnew = __nowide_prep_path_for_syscall_r(ptr, new);
    if (wnew == NULL) {
        free(wold);
        return -1;
    }

    short ret = _wrename(wold, wnew);
    free(wold);
    free(wnew);

    if (ret < 0) {
        _REENT_ERRNO(ptr) = __muteki_kerrno_to_errno(_GetLastError());
        return -1;
    }

    _REENT_ERRNO(ptr) = 0;
    return 0;
}
