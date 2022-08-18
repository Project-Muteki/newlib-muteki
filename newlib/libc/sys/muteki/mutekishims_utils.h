// Helpers and utilities for interacting with muteki-shims

#ifndef __MUTEKI_LIBC_MUTEKISHIMS_UTILS_H__
#define __MUTEKI_LIBC_MUTEKISHIMS_UTILS_H__

#include <muteki/errno.h>

void _init_muteki_io(void); // in io.c
void _free_muteki_io(void); // in io.c

int __muteki_kerrno_to_errno(kerrno_t kerrno);

#endif //__MUTEKI_LIBC_MUTEKISHIMS_UTILS_H__
