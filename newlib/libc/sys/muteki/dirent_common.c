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

#include <sys/dirent.h>
#include <sys/reent.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <muteki/common.h>
#include <muteki/fs.h>
#include "bestadescriptor.h"
#include "mutekishims_utils.h"
#include "nowide.h"

struct DIR_ {
    int fd;
    struct dirent dir;
    int index;
};

int closedir(DIR *dirp);
int dirfd(DIR *dirp);
DIR *__opendir_common(int fd);
DIR *opendir(const char *dirname);
DIR *fdopendir(int fd);
struct dirent *readdir(DIR *dirp);
int readdir_r(DIR *restrict dirp, struct dirent *restrict entry, struct dirent **restrict result);
void rewinddir(DIR *dirp);
int alphasort(const struct dirent **d1, const struct dirent **d2);
int scandir(const char *path, struct dirent ***res,
            int (*sel)(const struct dirent *),
            int (*cmp)(const struct dirent **, const struct dirent **));
void seekdir(DIR *dirp, long int index);
long int telldir(DIR *dirp);
