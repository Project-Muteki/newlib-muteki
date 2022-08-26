#include "applet_lifecycle.h"

extern int main(int argc, char *argv[]);

__attribute__((weak))
int applet_main(char *dospath, int subroutine, void *applet_arg1, void *applet_arg2) {
    int rv = 0;
    switch (subroutine) {
        case APPLET_SUBROUTINE_MAIN: {
            // TODO populate argv
            // TODO is there a way to pass multiple arguments?
            // More samples are probably needed (both system apps and games downloadable
            // from besta.com.tw) in order to make a final decision on this.
            //
            // Seems like not. However we do have the choice of having our own argv
            // protocol. Something like
            // ExecuteProgram?(&app, 0x4d544b50, { .argc = <argc>, .argvp = { &<argv[0]>, ... } }, 0)
            // would probably be enough. Any other types should follow the Besta
            // convention as closely as possible.
            char *argv[1] = { dospath };
            rv = main(1, argv);
            break;
        }
        case APPLET_SUBROUTINE_RESET_STATES: {
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
