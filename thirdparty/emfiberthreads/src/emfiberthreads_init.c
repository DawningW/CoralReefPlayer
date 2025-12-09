/*
 * Copyright (C) 2024 Yahweasel and contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <emscripten.h>
#include <errno.h>

#include "pthread-internal.h"

#ifndef EMFIBERTHREADS_ASYNCIFY_STACK_SIZE
#define EMFIBERTHREADS_ASYNCIFY_STACK_SIZE 4096
#endif

int emfiberthreads_init() {
    if (emfiberthreads_self)
        return 0;

    /* Not currently running fibers. Initialize the main fiber. */
    /* No main fiber. Initialize it. */
    emfiberthreads_mainFiber.list.next = &emfiberthreads_mainFiber;
    emfiberthreads_mainFiber.list.prev = &emfiberthreads_mainFiber;

    if (!emfiberthreads_mainFiber.asyncifyStack) {
        /* No asyncify stack. */
        emfiberthreads_mainFiber.asyncifyStack =
            calloc(EMFIBERTHREADS_ASYNCIFY_STACK_SIZE, 1);
        if (emfiberthreads_mainFiber.asyncifyStack == NULL)
            return errno;
    }

    emscripten_fiber_init_from_current_context(
        &emfiberthreads_mainFiber.fiber,
        emfiberthreads_mainFiber.asyncifyStack,
        EMFIBERTHREADS_ASYNCIFY_STACK_SIZE
    );

    emfiberthreads_self = emfiberthreads_next = &emfiberthreads_mainFiber;

    /* When a fiber ends the program, Emscripten can leave it in a broken state. */
    EM_ASM({ Fibers.trampolineRunning = false; });

    return 0;
}
