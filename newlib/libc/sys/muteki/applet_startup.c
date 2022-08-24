#include "applet_lifecycle.h"

__attribute__((weak))
int applet_startup(uintptr_t v1, uintptr_t v2, uintptr_t v3) {
    int type = (int) v1;
    const app_exec_context_t *ctx = (const app_exec_context_t *) v2;

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
        // I have no idea what v3 is used for in a Besta applet so just leave it alone.
    }

    return applet_main(subroutine, argc, argv);
}
