#include <fcntl.h>
#include <sys/unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <muteki/errno.h>
#include <muteki/file.h>
#include <muteki/fs.h>
#include <muteki/threading.h>

#include "bestadescriptor.h"
#include "mutekishims_utils.h"

DescriptorTranslation *__muteki_fdmap[MAX_OPEN_FILES];
DescriptorTranslation __muteki_fdmap_pool[MAX_OPEN_FILES];

critical_section_t _newlib_fd_mutex;

void _init_muteki_io(void) {
    OSInitCriticalSection(&_newlib_fd_mutex);

    OSEnterCriticalSection(&_newlib_fd_mutex);

    memset(__muteki_fdmap, 0, sizeof(__muteki_fdmap));
    memset(__muteki_fdmap_pool, 0, sizeof(__muteki_fdmap_pool));

    // stub stdin/out/err
    // stdout/err may be connected to WriteComDebugMsg in the future
    __muteki_fdmap[STDIN_FILENO] = &__muteki_fdmap_pool[STDIN_FILENO];
    __muteki_fdmap[STDIN_FILENO]->handle = NULL;
    __muteki_fdmap[STDIN_FILENO]->type = MUTEKI_DESCRIPTOR_DEVNULL;
    __muteki_fdmap[STDIN_FILENO]->ref_count = 1;

    __muteki_fdmap[STDOUT_FILENO] = &__muteki_fdmap_pool[STDOUT_FILENO];
    __muteki_fdmap[STDOUT_FILENO]->handle = NULL;
    __muteki_fdmap[STDOUT_FILENO]->type = MUTEKI_DESCRIPTOR_DEVNULL;
    __muteki_fdmap[STDOUT_FILENO]->ref_count = 1;

    __muteki_fdmap[STDERR_FILENO] = &__muteki_fdmap_pool[STDERR_FILENO];
    __muteki_fdmap[STDERR_FILENO]->handle = NULL;
    __muteki_fdmap[STDERR_FILENO]->type = MUTEKI_DESCRIPTOR_DEVNULL;
    __muteki_fdmap[STDERR_FILENO]->ref_count = 1;

    OSLeaveCriticalSection(&_newlib_fd_mutex);
}

void _free_muteki_io(void) {
    OSEnterCriticalSection(&_newlib_fd_mutex);

    if (__muteki_fdmap[STDIN_FILENO]) {
        memset(__muteki_fdmap[STDIN_FILENO], 0, sizeof(DescriptorTranslation));
        __muteki_fdmap[STDIN_FILENO] = NULL;
    }
    if (__muteki_fdmap[STDOUT_FILENO]) {
        memset(__muteki_fdmap[STDOUT_FILENO], 0, sizeof(DescriptorTranslation));
        __muteki_fdmap[STDOUT_FILENO] = NULL;
    }
    if (__muteki_fdmap[STDERR_FILENO]) {
        memset(__muteki_fdmap[STDERR_FILENO], 0, sizeof(DescriptorTranslation));
        __muteki_fdmap[STDERR_FILENO] = NULL;
    }

    OSLeaveCriticalSection(&_newlib_fd_mutex);

    OSDeleteCriticalSection(&_newlib_fd_mutex);
}

int __muteki_acquire_descriptor(void) {
    int fd = -1;
    int i = 0;
    OSEnterCriticalSection(&_newlib_fd_mutex);

    // get free descriptor
    // only allocate descriptors after stdin/stdout/stderr -> aka 0/1/2
    for (fd = 3; fd < MAX_OPEN_FILES; ++fd) {
        if (__muteki_fdmap[fd] == NULL) {
            // get free pool
            for (i = 0; i < MAX_OPEN_FILES; ++i) {
                if (__muteki_fdmap_pool[i].ref_count == 0) {
                    __muteki_fdmap[fd] = &__muteki_fdmap_pool[i];
                    __muteki_fdmap[fd]->ref_count = 1;
                    OSLeaveCriticalSection(&_newlib_fd_mutex);
                    return fd;
                }
            }
        }
    }

    // no mores descriptors available...
    OSLeaveCriticalSection(&_newlib_fd_mutex);
    return -1;
}

int __muteki_release_descriptor(int fd) {
    DescriptorTranslation *map = NULL;
    int res = -1;

    OSEnterCriticalSection(&_newlib_fd_mutex);

    if (is_fd_valid(fd) && __muteki_fd_drop(__muteki_fdmap[fd]) >= 0)
    {
        __muteki_fdmap[fd] = NULL;
        res = 0;
    }

    OSLeaveCriticalSection(&_newlib_fd_mutex);
    return res;
}

int __muteki_duplicate_descriptor(int fd) {
    int fd2 = -1;

    OSEnterCriticalSection(&_newlib_fd_mutex);

    if (is_fd_valid(fd)) {
        // get free descriptor
        // only allocate descriptors after stdin/stdout/stderr -> aka 0/1/2
        for (fd2 = 3; fd2 < MAX_OPEN_FILES; ++fd2) {
            if (__muteki_fdmap[fd2] == NULL) {
                __muteki_fdmap[fd2] = __muteki_fdmap[fd];
                __muteki_fdmap[fd2]->ref_count++;
                OSLeaveCriticalSection(&_newlib_fd_mutex);
                return fd2;
            }
        }
    }

    OSLeaveCriticalSection(&_newlib_fd_mutex);
    return -1;
}

int __muteki_descriptor_ref_count(int fd) {
    int res = 0;
    OSEnterCriticalSection(&_newlib_fd_mutex);
    res = __muteki_fdmap[fd]->ref_count;
    OSLeaveCriticalSection(&_newlib_fd_mutex);
    return res;
}

DescriptorTranslation *__muteki_fd_grab(int fd) {
    DescriptorTranslation *map = NULL;

    OSEnterCriticalSection(&_newlib_fd_mutex);

    if (is_fd_valid(fd)) {
        map = __muteki_fdmap[fd];
        if (map) map->ref_count++;
    }

    OSLeaveCriticalSection(&_newlib_fd_mutex);
    return map;
}

int __muteki_fd_drop(DescriptorTranslation *map) {
    OSEnterCriticalSection(&_newlib_fd_mutex);

    if (map->ref_count == 1) {
        int ret = 0;

        switch (map->type) {
            case MUTEKI_DESCRIPTOR_FILE: {
                ret = _fclose(map->handle);
                if (map->filename) {
                    free(map->filename);
                }
                break;
            }
            case MUTEKI_DESCRIPTOR_DIRECTORY: {
                ret = _findclose((find_context_t *) map->handle);
                if (map->filename) {
                    free(map->filename);
                }
                break;
            }
        }

        if (ret != 0) {
            muteki_errno_t berrno = _GetLastError();
            OSLeaveCriticalSection(&_newlib_fd_mutex);
            return -__muteki_besta_errno_to_errno(berrno);
        }

        map->ref_count--;
        memset(map, 0, sizeof(DescriptorTranslation));
    } else {
        map->ref_count--;
    }

    OSLeaveCriticalSection(&_newlib_fd_mutex);
    return 0;
}
