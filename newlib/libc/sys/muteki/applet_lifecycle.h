#ifndef __MUTEKI_LIBC_APPLET_LIFECYCLE_H__
#define __MUTEKI_LIBC_APPLET_LIFECYCLE_H__

#include <setjmp.h>
#include <muteki/loader.h> // for GetCurrentPathA

#define APP_SUBROUTINE_MUTEKI_MAIN 0x4d544b50

typedef struct {
    char *dospath;
    int *invoke_subroutine;
    char **args;
    int *app_arg3;
} app_exec_context_t;

extern int __exit_value;
extern jmp_buf __exit_jmp_buf;

extern int _start_after_fix(int type, const app_exec_context_t *ctx, int arg3);

extern int app_startup(int type, const app_exec_context_t *ctx, int arg3)
extern int app_main(int subroutine, int argc, char *argv[]);
extern void app_reset();

#endif // __MUTEKI_LIBC_APPLET_LIFECYCLE_H__

