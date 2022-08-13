#include "nowide.h"
#include <errno.h>
#include <iconv.h>
#include <malloc.h>
#include <string.h>

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
    iconv_t ic = iconv_open("UTF-16LE", "UTF-8");
    if (ic < 0) {
        r->_errno = ENOSYS;
        return NULL;
    }

    size_t alen = strlen(a);
    size_t aremaining = alen;
    size_t wremaining = (alen + 1) * sizeof(UTF16);

    UTF16 *result = (UTF16 *) calloc(alen + 1, sizeof(UTF16));
    if (result == NULL) {
        iconv_close(ic);
        r->_errno = ENOMEM;
        return NULL;
    }

    UTF16 *w = result;

    /* Is this actually safe? iconv shouldn't touch the inbuf contents but why
       some platform and standard has the non-constant pointer? */
    size_t num_failed_convs = iconv(ic, (char **) &a, &aremaining, (char **) &w, &wremaining);

    if (num_failed_convs != 0 || aremaining != 0) {
        free(result);
        iconv_close(ic);
        r->_errno = ENOENT;
        return NULL;
    }

    iconv_close(ic);
    return result;
}
