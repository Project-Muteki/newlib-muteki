#ifndef __MUTEKI_LIBC_NOWIDE_H__
#define __MUTEKI_LIBC_NOWIDE_H__

#include <reent.h>

#include <muteki/common.h>

extern UTF16 *__nowide_path_a2w_r(struct _reent *r, const char *a);

#endif // __MUTEKI_LIBC_NOWIDE_H__
