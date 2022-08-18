#include "applet_lifecycle.h"
#include "bestadescriptor.h"

int __exit_value;
jmp_buf __exit_jmp_buf;

extern void __libc_init_array(void);
extern void __libc_fini_array(void);
extern int main(int argc, char *argv[]);

int _start_after_fix(int type, const app_exec_context_t *ctx, int arg3) {
    // Run initialization hooks
    // Clear BSS area
    // TODO is this actually necessary?
    //memset(__bss_start__, 0, (size_t) (__bss_end__ - __bss_start__));
    __libc_init_array();
    _init_muteki_io();

    // Save the execution context for exit() and start the app.
    if (!setjmp(__exit_jmp_buf)) {
        __exit_value = app_startup(type, ctx, arg3);
    }

    // Run cleanup hooks and return.
    _free_muteki_io();
    __libc_fini_array();

    return __exit_value;
}

__attribute__((weak))
int app_startup(int type, const app_exec_context_t *ctx, int arg3) {
    char *argv[2];
    int argc = 0;
    int subroutine = APP_SUBROUTINE_MAIN;

    // Populate argv
    // TODO is there a way to pass multiple arguments?
    // More samples are probably needed (both system apps and games downloadable
    // from besta.com.tw) in order to make a final decision on this.
    //
    // Seems like not. However we do have the choice of having our own argv
    // protocol. Something like
    // ExecuteProgram?(&app, 0x4d544b50, { .argc = <argc>, .argvp = { &<argv[0]>, ... } }, 0)
    // would probably be enough. Any other types should follow the Besta
    // convention as closely as possible.
    if (type != 4) {
        argc = 1;
        argv[0] = GetCurrentPathA();
        argv[1] = NULL;
    } else {
        argc = 2;
        argv[0] = ctx->dospath;
        argv[1] = *ctx->args;
        subroutine = *ctx->invoke_subroutine;
        // I have no idea what arg3 is used for so just leave it alone.
    }

    return app_main(subroutine, argc, argv);
}

__attribute__((weak))
int app_main(int subroutine, int argc, char *argv[]) {
    int rv = 0;
    switch (subroutine) {
        case APP_SUBROUTINE_MAIN: {
            rv = main(argc, argv);
            break;
        }
        case APP_SUBROUTINE_RESET_STATES: {
            // TODO where should we put this?
            app_reset();
            rv = 3;
            break;
        }
        default: {
            rv = 4;
            break;
        }
    }
    return rv;
}

__attribute__((weak))
void app_reset() { /* ... */ }

__attribute__((noreturn))
void _exit(int ret) {
    __exit_value = ret;
    longjmp(__exit_jmp_buf, 1);
    __builtin_unreachable();
}

