#include "applet_lifecycle.h"
#include "bestadescriptor.h"
#include "mutekishims_utils.h"

extern int main(int argc, char *argv[]);

__attribute__((weak))
int applet_startup(int type, const app_exec_context_t *ctx, int arg3) {
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

    return applet_main(subroutine, argc, argv);
}

__attribute__((weak))
int applet_main(int subroutine, int argc, char *argv[]) {
    int rv = 0;
    switch (subroutine) {
        case APP_SUBROUTINE_MAIN: {
            rv = main(argc, argv);
            break;
        }
        case APP_SUBROUTINE_RESET_STATES: {
            // TODO where should we put this?
            applet_reset();
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
void applet_reset() { /* ... */ }

__attribute__((noreturn))
void _exit(int ret) {
    __exit_value = ret;
    longjmp(__exit_jmp_buf, 1);
    __builtin_unreachable();
}

