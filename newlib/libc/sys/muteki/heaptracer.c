#include <sys/heaptracer.h>
#include <stdint.h>
#include <stddef.h>
#include <muteki/file.h>

// So we lose as little performance as possible when heap tracer is turned off.
#define _unlikely(x) __builtin_expect(!!(x), 0)

void *__heap_tracer_osfh = NULL;
static const char TRACE_START[4] = {'H', 'T', 'R', 'C'};

bool heaptracer_start() {
    if (__heap_tracer_osfh != NULL) {
        return false;
    }
    // ab+ doesn't seem to work
    __heap_tracer_osfh = _afopen("C:\\HEAPT.BIN", "rb+");
    if (__heap_tracer_osfh == NULL) {
        __heap_tracer_osfh = _afopen("C:\\HEAPT.BIN", "wb+");
        if (__heap_tracer_osfh == NULL) {
            return false;
        }
    }
    __fseek(__heap_tracer_osfh, 0, _SYS_SEEK_END);
    _fwrite(TRACE_START, 1, sizeof(TRACE_START), __heap_tracer_osfh);
    __fflush(__heap_tracer_osfh);
    return true;
}

bool heaptracer_stop() {
    if (__heap_tracer_osfh == NULL) {
        return false;
    }
    _fclose(__heap_tracer_osfh);
    __heap_tracer_osfh = NULL;
    return true;
}

void _heaptracer_on_malloc(void *p, size_t size) {
    uintptr_t r = (uintptr_t) p;
    size_t s = size;
    if (_unlikely(__heap_tracer_osfh != NULL)) {
        _fwrite(&r, 1, sizeof(r), __heap_tracer_osfh);
        _fwrite(&s, 1, sizeof(s), __heap_tracer_osfh);
        __fflush(__heap_tracer_osfh);
    }
}

void _heaptracer_on_free(void *p) {
    uintptr_t r = (uintptr_t) p;
    size_t s = 0;
    if (_unlikely(__heap_tracer_osfh != NULL)) {
        _fwrite(&r, 1, sizeof(r), __heap_tracer_osfh);
        _fwrite(&s, 1, sizeof(s), __heap_tracer_osfh);
        __fflush(__heap_tracer_osfh);
    }
}

void _heaptracer_on_realloc(void *oldp, void *newp, size_t newsize) {
    uintptr_t r0 = (uintptr_t) newp;
    size_t s0 = newsize;
    uintptr_t r1 = (uintptr_t) oldp;
    size_t s1 = 0;
    if (_unlikely(__heap_tracer_osfh != NULL)) {
        if (newsize != 0) {
            _fwrite(&r0, 1, sizeof(r0), __heap_tracer_osfh);
            _fwrite(&s0, 1, sizeof(s0), __heap_tracer_osfh);
        }
        if (oldp != NULL) {
            _fwrite(&r1, 1, sizeof(r1), __heap_tracer_osfh);
            _fwrite(&s1, 1, sizeof(s1), __heap_tracer_osfh);
        }
        __fflush(__heap_tracer_osfh);
    }
}
