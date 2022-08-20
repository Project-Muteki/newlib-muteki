#include "nowide.h"
#include <errno.h>
#include <malloc.h>
#include <string.h>

extern int mutekix_console_printf(const char *fmt, ...);

// These are ported from newlib
size_t __nowide_bestawcslen(const UTF16 *s) {
    const UTF16 *p;

    p = s;
    while (*p)
        p++;

    return p - s;
}

// TODO: make this locale aware?
static int __utf8_mbtobestawc(struct _reent *r, UTF16 *pwc, const char *s, size_t n, __nowide_mbstate_t *state) {
    UTF16 dummy;
    unsigned char *t = (unsigned char *)s;
    int ch;
    int i = 0;

    if (pwc == NULL)
        pwc = &dummy;

    if (s == NULL)
        return 0;

    if (n == 0)
        return -2;

    if (state->__count == 0)
        ch = t[i++];
    else
        ch = state->__value.__wchb[0];

    if (ch == '\0') {
        *pwc = 0;
        state->__count = 0;
        return 0; /* s points to the null character */
    }

    if (ch <= 0x7f) {
        /* single-byte sequence */
        state->__count = 0;
        *pwc = ch;
        return 1;
    }
    if (ch >= 0xc0 && ch <= 0xdf) {
        /* two-byte sequence */
        state->__value.__wchb[0] = ch;
        if (state->__count == 0)
            state->__count = 1;
        else if (n < (size_t)-1)
            ++n;
        if (n < 2)
            return -2;
        ch = t[i++];
        if (ch < 0x80 || ch > 0xbf) {
            _REENT_ERRNO(r) = EILSEQ;
            return -1;
        }
        if (state->__value.__wchb[0] < 0xc2) {
            /* overlong UTF-8 sequence */
            _REENT_ERRNO(r) = EILSEQ;
            return -1;
        }
        state->__count = 0;
        *pwc = (UTF16)((state->__value.__wchb[0] & 0x1f) << 6)
               |    (UTF16)(ch & 0x3f);
        return i;
    }
    if (ch >= 0xe0 && ch <= 0xef) {
        /* three-byte sequence */
        UTF16 tmp;
        state->__value.__wchb[0] = ch;
        if (state->__count == 0)
            state->__count = 1;
        else if (n < (size_t)-1)
            ++n;
        if (n < 2)
            return -2;
        ch = (state->__count == 1) ? t[i++] : state->__value.__wchb[1];
        if (state->__value.__wchb[0] == 0xe0 && ch < 0xa0) {
            /* overlong UTF-8 sequence */
            _REENT_ERRNO(r) = EILSEQ;
            return -1;
        }
        if (ch < 0x80 || ch > 0xbf) {
            _REENT_ERRNO(r) = EILSEQ;
            return -1;
        }
        state->__value.__wchb[1] = ch;
        if (state->__count == 1)
            state->__count = 2;
        else if (n < (size_t)-1)
            ++n;
        if (n < 3)
            return -2;
        ch = t[i++];
        if (ch < 0x80 || ch > 0xbf) {
            _REENT_ERRNO(r) = EILSEQ;
            return -1;
        }
        state->__count = 0;
        tmp = (UTF16)((state->__value.__wchb[0] & 0x0f) << 12)
              |    (UTF16)((state->__value.__wchb[1] & 0x3f) << 6)
              |     (UTF16)(ch & 0x3f);
        *pwc = tmp;
        return i;
    }
    if (ch >= 0xf0 && ch <= 0xf4) {
        /* four-byte sequence */
        uint32_t tmp;
        state->__value.__wchb[0] = ch;
        if (state->__count == 0)
            state->__count = 1;
        else if (n < (size_t)-1)
            ++n;
        if (n < 2)
            return -2;
        ch = (state->__count == 1) ? t[i++] : state->__value.__wchb[1];
        if ((state->__value.__wchb[0] == 0xf0 && ch < 0x90)
                || (state->__value.__wchb[0] == 0xf4 && ch >= 0x90)) {
            /* overlong UTF-8 sequence or result is > 0x10ffff */
            _REENT_ERRNO(r) = EILSEQ;
            return -1;
        }
        if (ch < 0x80 || ch > 0xbf) {
            _REENT_ERRNO(r) = EILSEQ;
            return -1;
        }
        state->__value.__wchb[1] = ch;
        if (state->__count == 1)
            state->__count = 2;
        else if (n < (size_t)-1)
            ++n;
        if (n < 3)
            return -2;
        ch = (state->__count == 2) ? t[i++] : state->__value.__wchb[2];
        if (ch < 0x80 || ch > 0xbf) {
            _REENT_ERRNO(r) = EILSEQ;
            return -1;
        }
        state->__value.__wchb[2] = ch;
        if (state->__count == 2)
            state->__count = 3;
        else if (n < (size_t)-1)
            ++n;
        if (state->__count == 3 && sizeof(UTF16) == 2) {
            /* On systems which have UTF16 being UTF-16 values, the value
               doesn't fit into a single UTF16 in this case.  So what we
               do here is to store the state with a special value of __count
               and return the first half of a surrogate pair.  The first
               three bytes of a UTF-8 sequence are enough to generate the
               first half of a UTF-16 surrogate pair.  As return value we
               choose to return the number of bytes actually read up to
               here.
               The second half of the surrogate pair is returned in case we
               recognize the special __count value of four, and the next
               byte is actually a valid value.  See below. */
            tmp = (uint32_t)((state->__value.__wchb[0] & 0x07) << 18)
                  |   (uint32_t)((state->__value.__wchb[1] & 0x3f) << 12)
                  |   (uint32_t)((state->__value.__wchb[2] & 0x3f) << 6);
            state->__count = 4;
            *pwc = 0xd800 | ((tmp - 0x10000) >> 10);
            return i;
        }
        if (n < 4)
            return -2;
        ch = t[i++];
        if (ch < 0x80 || ch > 0xbf) {
            _REENT_ERRNO(r) = EILSEQ;
            return -1;
        }
        tmp = (uint32_t)((state->__value.__wchb[0] & 0x07) << 18)
              |   (uint32_t)((state->__value.__wchb[1] & 0x3f) << 12)
              |   (uint32_t)((state->__value.__wchb[2] & 0x3f) << 6)
              |   (uint32_t)(ch & 0x3f);
        if (state->__count == 4 && sizeof(UTF16) == 2)
            /* Create the second half of the surrogate pair for systems with
               UTF16 == UTF-16 . */
            *pwc = 0xdc00 | (tmp & 0x3ff);
        else
            *pwc = tmp;
        state->__count = 0;
        return i;
    }

    _REENT_ERRNO(r) = EILSEQ;
    return -1;
}

size_t __nowide_mbstobestawcs_r (struct _reent *r, UTF16 *__restrict pwcs, const char *__restrict s, size_t n, __nowide_mbstate_t *state) {
    size_t ret = 0;
    char *t = (char *)s;
    int bytes;

    if (!pwcs)
        n = (size_t) 1; /* Value doesn't matter as long as it's not 0. */
    while (n > 0) {
        bytes = __utf8_mbtobestawc (r, pwcs, t, 4, state);
        if (bytes < 0) {
            state->__count = 0;
            return -1;
        } else if (bytes == 0)
            break;
        t += bytes;
        ++ret;
        if (pwcs) {
            ++pwcs;
            --n;
        }
    }
    return ret;
}

// Shortcuts based on the above
/**
 * @brief Convert a UTF-8 "ANSI" path string to UTF-16 (Besta Unicode) with allocation.
 *
 * Sets errno to ENOSYS if the iconv codec is not provided, ENOMEM if any allocation fails or ENOENT if conversion fails.
 *
 * @param r Reentrent context.
 * @param a Source string in UTF-8.
 * @return The converted string in UTF-16 or NULL if conversion fails. The user needs to free it after use.
 */
UTF16 *__nowide_path_a2w_r(struct _reent *r, const char *a) {
    // TODO convert a into abspath
    size_t alen = strlen(a);
    __nowide_mbstate_t ctx = {0};

    UTF16 *result = (UTF16 *) calloc(alen + 1, sizeof(UTF16));
    if (result == NULL) {
        _REENT_ERRNO(r) = ENOMEM;
        return NULL;
    }

    UTF16 *w = result;

    int actual = __nowide_mbstobestawcs_r(r, w, a, alen, &ctx);

    if (actual < 0) {
        free(result);
        return NULL;
    }

    return result;
}
