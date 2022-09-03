#include "applet_lifecycle.h"

__attribute__((noreturn))
void _exit(int ret) {
    __exit_value = ret;
    longjmp(__exit_jmp_buf, 1);
    __builtin_unreachable();
}

