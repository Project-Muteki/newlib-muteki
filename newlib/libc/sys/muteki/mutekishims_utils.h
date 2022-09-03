// Helpers and utilities for interacting with muteki-shims

#ifndef __MUTEKI_LIBC_MUTEKISHIMS_UTILS_H__
#define __MUTEKI_LIBC_MUTEKISHIMS_UTILS_H__

#include <muteki/errno.h>

extern void _init_muteki_io(void); // in io.c
extern void _free_muteki_io(void); // in io.c

extern int __muteki_kerrno_to_errno(kerrno_t kerrno);
extern UTF16 *__muteki_dir_to_fnmatch(const UTF16 *path);

extern char *__realpath(const char *path);

#endif //__MUTEKI_LIBC_MUTEKISHIMS_UTILS_H__
