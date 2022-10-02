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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <muteki/memory.h>

typedef struct {
    size_t usable_size;
    void *raw_ptr;
} __mchx_t;

static const size_t __OVER_ALLOC_SIZE = 4 + sizeof(__mchx_t);

/**
 * @brief A hack that fixes allocator alignment by adding an extra header to allocated memchunk.
 *
 * Besta allocator is 4-bytes aligned but EABI requires 8-bytes aligned allocations. We can use compiler directives to cover this up but it might introduce performance penalty. This essentially over-allocates the memory and makes sure that there are at least 8 bytes available for us to store the original pointer and the allocation size so we don't have to resort to using the memchunk header to determine whether we're at the original pointer or not.
 *
 * @param q Unaligned pointer given by Besta lmalloc().
 * @param size Usable size of the allocation.
 * @return Pointer that aligns to 8-bytes.
 */
static inline void *__mchx_format(void *q, size_t size) {
    uintptr_t pp = ((((uintptr_t) q) + __OVER_ALLOC_SIZE) & (~((uintptr_t) 7u)));
    __mchx_t *mchx = (__mchx_t *) (pp - sizeof(__mchx_t));
    mchx->raw_ptr = q;
    mchx->usable_size = size;
    return (void *) pp;
}

/**
 * @brief Read size value from an mchx header.
 *
 * @param p Pointer produced by __mchx_format().
 * @return Size of usable memory.
 */
static inline size_t __mchx_get_size(void *p) {
    uintptr_t pp = (uintptr_t) p;
    __mchx_t *mchx = (__mchx_t *) (pp - sizeof(__mchx_t));
    return mchx->usable_size;
}

/**
 * @brief Read raw pointer value from an mchx header.
 *
 * @param p Pointer produced by __mchx_format().
 * @return The original unaligned pointer.
 */
static inline void *__mchx_get_raw(void *p) {
    uintptr_t pp = (uintptr_t) p;
    __mchx_t *mchx = (__mchx_t *) (pp - sizeof(__mchx_t));
    return mchx->raw_ptr;
}

/**
 * @brief Allocate and format mchx memchunk.
 *
 * Sets @p errno to @p ENOMEM if lmalloc() returns a NULL.
 *
 * @param r Newlib reentrant context.
 * @param size Size of the memory to allocate.
 * @return Pointer to allocated memory, or NULL if allocation fails.
 */
void *_malloc_r(struct _reent *r, size_t size) {
    void *q = lmalloc(size + __OVER_ALLOC_SIZE);

    if (q == NULL) {
        _REENT_ERRNO(r) = ENOMEM;
        return NULL;
    }

    return __mchx_format(q, size);
}

/**
 * @brief Allocate and clear memory.
 *
 * Sets @p errno to @p ENOMEM if _malloc_r() returns a NULL.
 *
 * @param r Newlib reentrant context.
 * @param nmemb Number of data units to allocate.
 * @param size Data unit size.
 * @return Pointer to allocated memory, or NULL if allocation fails.
 */
void *_calloc_r(struct _reent *r, size_t nmemb, size_t size) {
    void *p = _malloc_r(r, nmemb * size);

    if (p == NULL) {
        _REENT_ERRNO(r) = ENOMEM;
        return NULL;
    }

    memset(p, 0, nmemb * size);
    return p;
}

/**
 * @brief Reallocate a mchx allocation.
 *
 * Sets @p errno to @p ENOMEM if _malloc_r() fails.
 *
 * @param r Newlib reentrant context.
 * @param ptr Pointer to previously allocated memory with mchx.
 * @param size Size of the new memory.
 * @return Pointer to the new allocated memory, or NULL if allocation fails.
 */
void *_realloc_r(struct _reent *r, void *ptr, size_t size) {
    size_t orig_size = __mchx_get_size(ptr);
    size_t copy_size = (size <= orig_size) ? size : orig_size;

    void *p = _malloc_r(r, size);
    if (p == NULL) {
        _REENT_ERRNO(r) = ENOMEM;
        return NULL;
    }

    memcpy(p, ptr, copy_size);
    _free_r(r, ptr);
    return p;
}

/**
 * @brief Free a mchx memchunk.
 *
 * @param r Newlib reentrant context.
 * @param ptr Pointer to previously allocated memory.
 */
void _free_r(struct _reent *r, void *ptr) {
    if (ptr == NULL) {
        return;
    }
    _lfree(__mchx_get_raw(ptr));
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
