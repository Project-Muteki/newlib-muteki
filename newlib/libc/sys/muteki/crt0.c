#include <string.h>
#include "applet_lifecycle.h"
#include "mutekishims_utils.h"

int __stack_fixed = 0;
int __exit_value;
jmp_buf __exit_jmp_buf;

extern void __libc_init_array(void);
extern void __libc_fini_array(void);

int _start_after_fix(uintptr_t v1, uintptr_t v2, uintptr_t v3) {
    // Run initialization hooks
    __libc_init_array();
    _init_muteki_io();

    // Save the execution context for exit() and start the app.
    if (!setjmp(__exit_jmp_buf)) {
        __exit_value = applet_startup(v1, v2, v3);
    }

    // Run cleanup hooks and return.
    _free_muteki_io();
    __libc_fini_array();

    return __exit_value;
}

/* Works around a Besta RTOS program loader bug that resulted in unaligned stack
 * pointer being handed over to the application */
__attribute__((naked))
int _start(int type, const app_exec_context_t *ctx, int arg3) {
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
