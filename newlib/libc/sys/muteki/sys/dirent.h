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

/* This is initially based on vitasdk dirent.h but the actual struct
   deviated significantly later due to implementation differences. */

#ifndef _SYS_DIRENT_H_
#define	_SYS_DIRENT_H_

#include <sys/types.h>
#include <sys/time.h>
#include <sys/dirent.h>

#ifndef _DIRENT_HAVE_D_TYPE
#define _DIRENT_HAVE_D_TYPE
#endif

#ifndef _DIRENT_HAVE_D_RECLEN
#define _DIRENT_HAVE_D_RECLEN
#endif

#ifndef _DIRENT_HAVE_D_NAMLEN
#define _DIRENT_HAVE_D_NAMLEN
#endif

/* Some of these are needed by libstdc++. */
#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12

/* Roughly based on the BSD dirent with added fields. */
struct dirent {
    ino_t d_ino; /* Inode. not filled on Besta but kept for backwards compatibility. */
    off_t d_off; /* Directory entry offset. */
    unsigned char d_type; /* BSD entry type field. */
    unsigned char d_fatattrib; /* FAT attribute. Besta exclusive. */
    unsigned short d_reclen; /* Length of actual record. */
    unsigned short d_namlen; /* Length of entry name. */
    char d_name[768]; /* Entry name (big enough to contain 255 UTF-16 codepoints in UTF-8) */
};

struct DIR_;
typedef struct DIR_ DIR;

#endif /* _SYS_DIRENT_H_ */

