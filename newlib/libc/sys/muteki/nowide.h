#ifndef __MUTEKI_LIBC_NOWIDE_H__
#define __MUTEKI_LIBC_NOWIDE_H__

#include <reent.h>

#include <muteki/common.h>

typedef struct {
    int __count;
    union {
        uint32_t __wch;
        unsigned char __wchb[4];
    } __value;		/* Value so far.  */
} __nowide_mbstate_t;

extern size_t __nowide_mbstobestawcs_r(struct _reent *r, UTF16 *__restrict pwcs, const char *__restrict s, size_t n, __nowide_mbstate_t *state);
extern size_t __nowide_bestawcslen(const UTF16 *s);
extern UTF16 *__nowide_prep_path_for_syscall_r(struct _reent *r, const char *a);

#endif // __MUTEKI_LIBC_NOWIDE_H__
