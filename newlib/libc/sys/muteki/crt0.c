#include <string.h>
#include "applet_lifecycle.h"

static int __stack_fixed = 0;

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
