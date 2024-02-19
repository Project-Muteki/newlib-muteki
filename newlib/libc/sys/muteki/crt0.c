#include <stdlib.h>
#include <string.h>
#include <sys/reent.h>
#include "applet_lifecycle.h"
#include "mutekishims_utils.h"
#include "tls.h"
#include <muteki/threading.h>
#include <muteki/utils.h>

#ifdef _ENABLE_MUTEKI_LIBC_HEAP_TRACE
#include <sys/heaptracer.h>
#endif

int __exit_value;
jmp_buf __exit_jmp_buf;

extern void __libc_init_array(void);

static void zap_sglue(struct _glue *next) {
    if (next == NULL) {
        return;
    }
    if (next->_next != NULL) {
        zap_sglue(next->_next);
    }
    free(next);
}

// Clean up __sglue because Besta RTOS won't do it for us
static void goo_gone() {
    zap_sglue(__sglue._next);
}

static void __attribute__((destructor(1))) on_fini() {
    _free_muteki_io();
    goo_gone();
}

static void __attribute__((constructor(1))) on_init() {
    _init_muteki_io();
}

int _start_after_fix(int exec_proto_ver, applet_args_v4_t *app_ctx, uintptr_t _sbz) {
#ifdef _ENABLE_MUTEKI_LIBC_HEAP_TRACE
    heaptracer_start();
#endif

    // needed for main thread reent
    // Do TLS stuff as early as possible and absolutely before init/after fini or horrible things could happen
    // (use-after-free, memory leaks, etc.)
    mutekix_tls_init_self();
    
    // Run all initialization hooks
    __libc_init_array();

    // Save the execution context for exit() and start the app.
    if (!setjmp(__exit_jmp_buf)) {
        exit(applet_startup(exec_proto_ver, app_ctx, _sbz));
    }

    mutekix_tls_free_self(MUTEKIX_TLS_KEY_TLS);

#ifdef _ENABLE_MUTEKI_LIBC_HEAP_TRACE
    heaptracer_stop();
#endif
    return __exit_value;
}

// Ensure 8-byte stack alignment for EABI compatibility.
// Note: This actually has arguments and return value but they are not declared here to prevent potential issue with naked functions. See _start_after_fix for the declaration.
__attribute__((naked))
void _start() {
    // WARNING: Do NOT clobber r0-r2 here or bad thing could happen!
    asm volatile (
        // Align to 8-bytes ourselves
        "push {r4, lr}\n\t"
        // Check for alignment
        "tst sp, #7\n\t"
        // If not aligned, the dummy value flag itself will ensure the alignment, otherwise push a dummy value to maintain the alignment.
        "mov r4, #0\n\t"
        "subeq sp, sp, #4\n\t"
        // Flag is set when dummy value is pushed.
        "moveq r4, #1\n\t"
        // Push the flag value
        "stmfd sp!, {r4}\n\t"
        // Run the actual start routine
        "bl _start_after_fix\n\t"
        // After finished, pop the dummy value flag and check it
        "ldmfd sp!, {r4}\n\t"
        "cmp r4, #0\n\t"
        // If set, pop the dummy value.
        "addne sp, sp, #4\n\t"
        // Return
        "pop {r4, pc}"
    );
}
