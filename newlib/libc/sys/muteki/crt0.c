#include <stdlib.h>
#include <string.h>
#include <sys/reent.h>
#include "applet_lifecycle.h"
#include "mutekishims_utils.h"

int __stack_fixed = 0;
int __exit_value;
jmp_buf __exit_jmp_buf;

extern void __libc_init_array(void);

static void zap_sglue(struct _glue *next) {
    if (next == NULL) {
        return;
    }
    if (next->_next != NULL) {
        zap_sglue(next->_next);
    } else {
        free(next);
    }
}

// Clean up __sglue because Besta RTOS won't do it for us
static void goo_gone() {
    zap_sglue(__sglue._next);
}

static void __attribute__((constructor(1))) on_init() {
    _init_muteki_io();

    // Register cleanup hooks to be run by exit()
    // TODO since they need to be run all the time, should we use __attribute__((destructor(x))) for this?
    atexit(&_free_muteki_io);
    atexit(&goo_gone);
}

int _start_after_fix(int exec_proto_ver, applet_args_v4_t *app_ctx, uintptr_t _sbz) {
    // Run all initialization hooks
    __libc_init_array();

    // Save the execution context for exit() and start the app.
    if (!setjmp(__exit_jmp_buf)) {
        exit(applet_startup(exec_proto_ver, app_ctx, _sbz));
    }
    return __exit_value;
}

/* Works around a Besta RTOS program loader bug that resulted in unaligned stack
 * pointer being handed over to the application */
__attribute__((naked))
int _start(int exec_proto_ver, applet_args_v4_t *applet_args, uintptr_t _sbz) {
    asm (
        // Align to 8-bytes ourselves
        "push {r4, r5, r6, lr}\n\t"
        // Check for alignment
        "bic r4, sp, #7\n\t"
        "cmp r4, sp\n\t"
        // Fix and set the flag if not aligned
        "addne sp, sp, #-4\n\t"
        "movne r4, #1\n\t"
        "ldrne r5, =__stack_fixed\n\t"
        "strne r4, [r5]\n\t"
        // Run the actual start routine
        "bl _start_after_fix\n\t"
        // After finished, check for the fixed flag
        "ldr r4, =__stack_fixed\n\t"
        "ldr r4, [r4]\n\t"
        "cmp r4, #1\n\t"
        // If set, restore the original stack pointer
        "addeq sp, sp, #4\n\t"
        // Return
        "pop {r4, r5, r6, lr}\n\t"
        "bx lr"
    );
}
