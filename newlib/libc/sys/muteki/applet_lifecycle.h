#ifndef __MUTEKI_LIBC_APPLET_LIFECYCLE_H__
#define __MUTEKI_LIBC_APPLET_LIFECYCLE_H__

#include <setjmp.h>
#include <stdint.h>
#include <muteki/loader.h> // for GetCurrentPathA

#define APP_SUBROUTINE_MUTEKI_MAIN 0x4d544b50


extern int __exit_value;
extern jmp_buf __exit_jmp_buf;

extern int _start_after_fix(int exec_proto_ver, applet_args_v4_t *applet_args, uintptr_t _sbz);

extern int applet_startup(int exec_proto_ver, applet_args_v4_t *applet_args, uintptr_t _sbz);
extern int applet_main(char *dospath, int subroutine, void *applet_arg1, void *applet_arg2);
extern void applet_reset();

#endif // __MUTEKI_LIBC_APPLET_LIFECYCLE_H__

