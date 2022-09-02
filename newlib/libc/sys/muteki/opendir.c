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

DIR *__opendir_common(int fd) {
    DIR *dirp = calloc(1, sizeof(DIR));

    if (!dirp) {
        errno = ENOMEM;
        return NULL;
    }

    dirp->fd = fd;
    dirp->index = 0;

    errno = 0;
    return dirp;
}

DIR *opendir(const char *dirname) {
    int fd;

    if ((fd = open(dirname, O_RDONLY | O_DIRECTORY)) == -1)
        return (NULL);
    DIR *ret = __opendir_common(fd);
    if (!ret) {
        close(fd);
    }
    return ret;
}

DIR *fdopendir(int fd) {
    DescriptorTranslation *fdmap = __muteki_fd_grab(fd);
    if (!fdmap) {
        errno = EBADF;
        return NULL;
    }

    if (fdmap->type != MUTEKI_DESCRIPTOR_DIRECTORY) {
        __muteki_fd_drop(fdmap);
        errno = ENOTDIR;
        return NULL;
    }

    __muteki_fd_drop(fdmap);

    return (__opendir_common(fd));
}
