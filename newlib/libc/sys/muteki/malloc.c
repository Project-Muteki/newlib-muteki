/**
 * @file malloc.c
 * @brief Heap memory management function hooks.
 *
 * Since Besta RTOS does not have virtual memory support, sbrk()-based heap
 * management in newlib will not work great. Spoofing a heap space with a huge
 * malloc() on the OS heap wastes memory and can potentially cause the OS to
 * become unstable. Therefore the solution here is to simply redirect basic
 * heap management functions (i.e. malloc(), calloc(), realloc() and free()) to
 * their Besta RTOS native counterpart so we don't need a custom newlib.
 *
 */

#include <_ansi.h>
#include <errno.h>
#include <reent.h>
#include <stdlib.h>
#include <malloc.h>

#include <muteki/memory.h>

/**
 * @brief Redirects _malloc_r to lmalloc.
 *
 * Sets @p errno to @p ENOMEM if lmalloc() returns a NULL.
 *
 * @param r Newlib reentrant context.
 * @param size Size of the memory to allocate.
 * @return Pointer to allocated memory, or NULL if allocation fails.
 */
void *_malloc_r(struct _reent *r, size_t size) {
    void *p = lmalloc(size);
    if (p == NULL) {
        r->_errno = ENOMEM;
    }
    return p;
}

/**
 * @brief Redirects _calloc_r to lcalloc.
 *
 * Sets @p errno to @p ENOMEM if lcalloc() returns a NULL.
 *
 * @param r Newlib reentrant context.
 * @param nmemb Data unit size.
 * @param size Number of data units to allocate.
 * @return Pointer to allocated memory, or NULL if allocation fails.
 */
void *_calloc_r(struct _reent *r, size_t nmemb, size_t size) {
    void *p = lcalloc(nmemb, size);
    if (p == NULL) {
        r->_errno = ENOMEM;
    }
    return p;
}

/**
 * @brief Redirects _realloc_r to lrealloc.
 *
 * Sets @p errno to @p ENOMEM if lrealloc() returns a NULL.
 *
 * @param r Newlib reentrant context.
 * @param ptr Pointer to previously allocated memory.
 * @param size Size of the new memory.
 * @return Pointer to the new allocated memory, or NULL if allocation fails.
 */
void *_realloc_r(struct _reent *r, void *ptr, size_t size) {
    void *p = lrealloc(ptr, size);
    if (p == NULL) {
        r->_errno = ENOMEM;
    }
    return p;
}

/**
 * @brief Redirects _free_r to _lfree.
 *
 * @param r Newlib reentrant context.
 * @param ptr Pointer to previously allocated memory.
 */
void _free_r(struct _reent *r, void *ptr) {
    _lfree(ptr);
}

/* Below are copied from newlib's reentrent to C standard functions. */

void *malloc(size_t nbytes) {
    return _malloc_r(_REENT, nbytes);
}

void free(void *aptr) {
    _free_r(_REENT, aptr);
}

void *calloc(size_t n, size_t size) {
    return _calloc_r(_REENT, n, size);
}

void *realloc(void *ap, size_t nbytes) {
    return _realloc_r(_REENT, ap, nbytes);
}
