#include "applet_lifecycle.h"

extern int main(int argc, char *argv[]);

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
