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

#include "pthread-internal.h"

int emfiber_pthread_yield() {
    emfiber_pthread_t self;

    EMFT_INIT();

    self = emfiberthreads_self;
    emfiberthreads_self = emfiberthreads_next;
    emfiberthreads_next = emfiberthreads_self->list.next;
    emscripten_fiber_swap(&self->fiber, &emfiberthreads_self->fiber);

    if (emfiberthreads_self == &emfiberthreads_mainFiber &&
        emfiberthreads_mainFiber.list.next == &emfiberthreads_mainFiber) {
        /* No more threads left, clean up. */
        emfiberthreads_self = emfiberthreads_next = NULL;
    }

    return 0;
}
