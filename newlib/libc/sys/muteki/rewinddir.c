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

void rewinddir(DIR *dirp) {
    find_context_t new_dirfd;

    if (!dirp) {
        errno = EBADF;
        return;
    }

    DescriptorTranslation *fdmap = __muteki_fd_grab(dirp->fd);
    if (!fdmap) {
        errno = EBADF;
        return;
    }

    UTF16 *fnmatch = __muteki_dir_to_fnmatch(fdmap->filename);

    if (fnmatch == NULL) {
        errno = EINVAL;
        return;
    }

    short find_result = _wfindfirst(fnmatch, &new_dirfd, 0);
    free(fnmatch);
    if (find_result < 0) {
        __muteki_fd_drop(fdmap);
        errno = __muteki_kerrno_to_errno(_GetLastError());
        return;
    }

    _findclose(fdmap->handle);
    memcpy(fdmap->handle, &new_dirfd, sizeof(new_dirfd));
    __muteki_fd_drop(fdmap);

    dirp->index = 0;
    errno = 0;
}
