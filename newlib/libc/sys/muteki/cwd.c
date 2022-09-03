/*

Copyright (C) 2021, vitasdk
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

#include <reent.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/syslimits.h>
#include <limits.h>
#include <string.h>

#include <muteki/errno.h>
#include <muteki/fs.h>

#include "bestadescriptor.h"
#include "mutekishims_utils.h"
#include "nowide.h"

static const char FALLBACK_DEFAULT_CWD[] = "C:";

static char __cwd[PATH_MAX] = {0};

// get end of drive position from full path
static int __get_drive(const char *path) {
    char first_char = toupper(path[0]);
    if (first_char >= 'A' && first_char <= 'Z' && path[1] == ':') {
        return 2;
    }
    if (path[0] == '\\' && path[1] == '\\' && path[3] == '\\' && (path[2] == '?' || path[2] == '.')) {
        return 3;
    }
    return 0;
}

static void __init_cwd() {
    __nowide_mbstate_t ctx = {0};
    UTF16 sys_cwd[SYS_PATH_MAX_CU];
    if (strlen(__cwd) == 0) {
        // init cwd
        if (_wgetcurdir(NULL, sys_cwd) < 0) {
            strcpy(__cwd, FALLBACK_DEFAULT_CWD);
            return;
        }
        size_t wcs2mbs_ret = __nowide_bestawcstombs_r(_REENT, __cwd, sys_cwd, sizeof(__cwd), &ctx);
        if (wcs2mbs_ret == (size_t) -1) {
            strcpy(__cwd, FALLBACK_DEFAULT_CWD);
            return;
        }
    }
}

char *getcwd(char *buf, size_t size) {
    __init_cwd();

    if (buf != NULL && size == 0) {
        errno = EINVAL;
        return NULL;
    }

    if(buf && strlen(__cwd) >= size) {
        errno = ERANGE;
        return NULL;
    }

    // latest POSIX says it's implementation-defined
    // musl, glibc and bionic allocs buffer if it's null.
    // so we'll do the same
    if (buf == NULL) {
        buf = (char *) calloc(PATH_MAX, sizeof(char));
        if (buf == NULL) {
            errno = ENOMEM;
            return NULL;
        }
    }

    strcpy(buf, __cwd);
    return buf;
}

// modified from bionic
char *__resolve_path(const char *path, char resolved[PATH_MAX]) {
    char *p, *q, *s;
    size_t left_len, resolved_len;
    char left[PATH_MAX], next_token[PATH_MAX];
    if (path[0] == '\\') {
        resolved[0] = '\\';
        resolved[1] = '\0';
        if (path[1] == '\0')
            return (resolved);
        resolved_len = 1;
        left_len = strlcpy(left, path + 1, sizeof(left));
    } else {
        resolved_len = 0;
        left_len = strlcpy(left, path, sizeof(left));
    }
    if (left_len >= sizeof(left) || resolved_len >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return (NULL);
    }
    /*
     * Iterate over path components in `left'.
     */
    while (left_len != 0) {
        /*
         * Extract the next path component and adjust `left'
         * and its length.
         */
        p = strchr(left, '\\');
        s = p ? p : left + left_len;
        if (s - left >= sizeof(next_token)) {
            errno = ENAMETOOLONG;
            return (NULL);
        }
        memcpy(next_token, left, s - left);
        next_token[s - left] = '\0';
        left_len -= s - left;
        if (p != NULL)
            memmove(left, s + 1, left_len + 1);
        if (resolved[resolved_len - 1] != '\\') {
            if (resolved_len + 1 >= PATH_MAX) {
                errno = ENAMETOOLONG;
                return (NULL);
            }
            resolved[resolved_len++] = '\\';
            resolved[resolved_len] = '\0';
        }
        if (next_token[0] == '\0')
            continue;
        else if (strcmp(next_token, ".") == 0)
            continue;
        else if (strcmp(next_token, "..") == 0) {
            /*
             * Strip the last path component except when we have
             * single "\\"
             */
            if (resolved_len > 1) {
                resolved[resolved_len - 1] = '\0';
                q = strrchr(resolved, '\\') + 1;
                *q = '\0';
                resolved_len = q - resolved;
            }
            continue;
        }
        /*
         * Append the next path component.
         */
        resolved_len = strlcat(resolved, next_token, PATH_MAX);
        if (resolved_len >= PATH_MAX) {
            errno = ENAMETOOLONG;
            return (NULL);
        }
    }
    /*
     * Remove trailing slash except when the resolved pathname
     * is a single "\\".
     */
    if (resolved_len > 1 && resolved[resolved_len - 1] == '\\')
        resolved[resolved_len - 1] = '\0';
    return (resolved);
}

// internal, without stat check. Used for mkdir/open/etc.
// TODO forward slashes to backward slashes
char *__realpath(const char *path) {
    char resolved[PATH_MAX] = {0};
    char result[PATH_MAX] = {0};
    char *resolved_path = NULL;

    // can't have null path
    if (!path) {
        errno = EINVAL;
        return NULL;
    }

    // empty path would resolve to cwd
    // POSIX.1-2008 states that ENOENT should be returned if path points to empty string
    if (strlen(path) == 0) {
        errno = ENOENT;
        return NULL;
    }

    resolved_path = (char *) calloc(PATH_MAX, sizeof(char));
    if (!resolved_path) {
        errno = ENOMEM;
        return NULL;
    }

    int d = __get_drive(path);
    if (d) { // absolute path with drive
        __resolve_path(path + d, resolved);
        if (strlen(resolved) + d < PATH_MAX) {
            strncpy(result, path, d);
            strcat(result, resolved);
            strcpy(resolved_path, result);
            return resolved_path;
        }
        errno = ENAMETOOLONG;
        free(resolved_path);
        return NULL;
    } else if (path[0] == '\\') { // absolute path without drive
        __init_cwd();
        __resolve_path(path, resolved);
        d = __get_drive(__cwd);
        if (strlen(resolved) + d < PATH_MAX) {
            strncpy(result, __cwd, d);
            strcat(result, resolved);
            strcpy(resolved_path, result);
            return resolved_path;
        }
        errno = ENAMETOOLONG;
        free(resolved_path);
        return NULL;
    } else { // relative path
        __init_cwd();
        char full_path[PATH_MAX] = {0};
        if (strlen(__cwd) + strlen(path) < PATH_MAX) {
            strcpy(full_path, __cwd);
            strcat(full_path, "\\");
            strcat(full_path, path);
            d = __get_drive(full_path);

            __resolve_path(full_path + d, resolved);
            if (strlen(resolved) + d < PATH_MAX) {
                strncpy(result, full_path, d);
                strcat(result, resolved);
                strcpy(resolved_path, result);
                return resolved_path;
            }
            errno = ENAMETOOLONG;
            free(resolved_path);
            return NULL;
        }
        errno = ENAMETOOLONG;
        free(resolved_path);
        return NULL;
    }
}

char *realpath(const char *path, char *resolved_path) {
    char *fullpath = __realpath(path);
    if (!fullpath) {
        return NULL; // errno already set
    }

    UTF16 *wfullpath = __nowide_mbstobestawcs_dup_r(_REENT, fullpath);
    if (wfullpath == NULL) {
        free(fullpath);
        return NULL; // errno already set
    }

    short fat_attrib = _wfgetattr(wfullpath);
    free(wfullpath);
    
    if (fat_attrib < 0) {
        free(fullpath);
        errno = __muteki_kerrno_to_errno(_GetLastError());
        return NULL;
    }

    if (!resolved_path) {
        resolved_path = fullpath;
    } else {
        strcpy(resolved_path, fullpath);
        free(fullpath);
    }

    return resolved_path;
}

int chdir(const char *path) {
    char *fullpath = NULL;

    if (!path) { // different error
        errno = EFAULT;
        return -1;
    }

    fullpath = __realpath(path);
    if (!fullpath) {
        return -1; // errno already set
    }

    UTF16 *wfullpath = __nowide_mbstobestawcs_dup_r(_REENT, fullpath);
    if (wfullpath == NULL) {
        free(fullpath);
        return -1; // errno already set
    }

    short fat_attrib = _wfgetattr(wfullpath);
    if (fat_attrib < 0) {
        free(fullpath);
        errno = __muteki_kerrno_to_errno(_GetLastError());
        return -1;
    }

    if (!(fat_attrib & ATTR_DIR)) {
        free(fullpath);
        errno = ENOTDIR;
        return -1;
    }

    strcpy(__cwd, fullpath);
    _wchdir(wfullpath);
    free(wfullpath);
    free(fullpath);
    return 0;
}

int fchdir(int fd) {
    int ret;
    DescriptorTranslation *fdmap = __muteki_fd_grab(fd);

    if (!fdmap) {
        errno = EBADF;
        return -1;
    }

    switch (fdmap->type) {
    case MUTEKI_DESCRIPTOR_DEVNULL:
    case MUTEKI_DESCRIPTOR_FILE:
        __muteki_fd_drop(fdmap);
        errno = ENOTDIR;
        return -1;
    case MUTEKI_DESCRIPTOR_DIRECTORY: {
        __nowide_mbstate_t ctx = {0};
        size_t ret = __nowide_bestawcstombs_r(_REENT, __cwd, fdmap->filename, sizeof(__cwd), &ctx);
        _wchdir(fdmap->filename);
        __muteki_fd_drop(fdmap);
        if (ret == ((size_t) -1)) {
            return -1;
        }
    }
    }
    return 0;
}
