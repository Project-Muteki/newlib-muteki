/*

Copyright (C) 2016, David "Davee" Morgan
Copyright (C) 2022, dogtopus

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include "dirent_common.c"

static int find_ctx_to_dirent(struct dirent *out, const find_context_t *in) {
    __nowide_mbstate_t mbstate = {0};

    out->d_fatattrib = in->attrib;
    if (in->attrib & ATTR_DIR) {
        out->d_type = DT_DIR;
    } else {
        out->d_type = DT_REG;
    }
    size_t filename_size = __nowide_bestawcstombs_r(_REENT, out->d_name, in->filename_lfn, sizeof(out->d_name), &mbstate);
    out->d_namlen = filename_size & 0xffff;
    out->d_reclen = out->d_namlen + (sizeof(*out) - sizeof(out->d_name));
    return 0;
}

struct dirent *readdir(DIR *dirp) {
    if (!dirp) {
        errno = EBADF;
        return NULL;
    }

    DescriptorTranslation *fdmap = __muteki_fd_grab(dirp->fd);
    if (!fdmap) {
        errno = EBADF;
        return NULL;
    }

    if (dirp->index > 0) {
        short res = _wfindnext((find_context_t *) fdmap->handle);
        if (res < 0) {
            kerrno_t kerrno = _GetLastError();
            __muteki_fd_drop(fdmap);
            // FS_NO_SUCH_ENTRY is raised on end of directory.
            if (KERRNO_NS(kerrno) != ERRNO_NS_KERNEL || KERRNO_ERR(kerrno) != FS_NO_SUCH_ENTRY) {
                errno = __muteki_kerrno_to_errno(kerrno);
            }
            return NULL;
        }
    }

    find_ctx_to_dirent(&dirp->dir, (find_context_t *) fdmap->handle);

    __muteki_fd_drop(fdmap);

    struct dirent *dir = &dirp->dir;
    dirp->index++;
    return dir;
}
