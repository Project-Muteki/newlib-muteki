#include <sys/types.h>

#include <muteki/threading.h>

static inline void _millis(unsigned long ms) {
    while (ms > 0x7fff) {
        OSSleep(0x7fff);
        ms -= 0x7fff;
    }
    OSSleep(ms);
}

unsigned int sleep(unsigned int seconds) {
    _millis(seconds * 1000);
    return 0;
}

unsigned int usleep(__useconds_t useconds) {
    _millis(useconds / 1000);
    return 0;
}

