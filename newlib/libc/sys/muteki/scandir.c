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

int alphasort(const struct dirent **d1, const struct dirent **d2) {
    return (strcoll((*d1)->d_name, (*d2)->d_name));
}

// modified from musl
int scandir(const char *path, struct dirent ***res,
            int (*sel)(const struct dirent *),
            int (*cmp)(const struct dirent **, const struct dirent **)) {
    DIR *d = opendir(path);
    struct dirent *de, **names=0, **tmp;
    size_t cnt=0, len=0;
    int old_errno = errno;

    if (!d) return -1;

    while ((errno=0), (de = readdir(d))) {
        if (sel && !sel(de)) continue;
        if (cnt >= len) {
            len = 2*len+1;
            if (len > SIZE_MAX/sizeof *names) break;
            tmp = realloc(names, len * sizeof *names);
            if (!tmp) break;
            names = tmp;
        }
        names[cnt] = malloc(sizeof(struct dirent));
        if (!names[cnt]) break;
        memcpy(names[cnt++], de, sizeof(struct dirent));
    }

    closedir(d);

    if (errno) {
        if (names) while (cnt-- > 0) free(names[cnt]);
        free(names);
        return -1;
    }
    errno = old_errno;

    if (cmp) qsort(names, cnt, sizeof *names, (int (*)(const void *, const void *))cmp);
    *res = names;
    return cnt;
}
