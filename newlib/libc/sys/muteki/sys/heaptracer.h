#ifndef __SYS_HEAPTRACER_H__
#define __SYS_HEAPTRACER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

extern bool heaptracer_start();
extern bool heaptracer_stop();

#ifdef __cplusplus
}
#endif

#endif
