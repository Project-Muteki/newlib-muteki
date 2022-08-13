/*

Copyright (C) 2016, David "Davee" Morgan
Copyright (C) 2022, dogtopus

Adapted from vitasdk's vitadescriptor.h.

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

#ifndef __MUTEKI_LIBC_BESTADESCRIPTOR_H__
#define __MUTEKI_LIBC_BESTADESCRIPTOR_H__


#define MAX_OPEN_FILES 256

typedef enum {
    MUTEKI_DESCRIPTOR_DEVNULL,
    MUTEKI_DESCRIPTOR_FILE,
    MUTEKI_DESCRIPTOR_DIRECTORY,
    MUTEKI_DESCRIPTOR_CHARDEV,
} DescriptorTypes;

typedef struct {
    void *handle;
    DescriptorTypes type;
    int ref_count;
    char* filename;
} DescriptorTranslation;

extern DescriptorTranslation *__muteki_fdmap[];


int __muteki_acquire_descriptor(void);
int __muteki_release_descriptor(int fd);
int __muteki_duplicate_descriptor(int fd);
int __muteki_descriptor_ref_count(int fd);
DescriptorTranslation *__muteki_fd_grab(int fd);
int __muteki_fd_drop(DescriptorTranslation *fdmap);

static inline int is_fd_valid(int fd) {
    return (fd > 0) && (fd < MAX_OPEN_FILES) && (__muteki_fdmap[fd] != NULL);
}

#endif // _MUTEKI_LIBC_BESTADESCRIPTOR_H_

