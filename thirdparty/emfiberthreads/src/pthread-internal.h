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

#ifndef EMFIBERTHREADS_PTHREAD_INTERNAL_H
#define EMFIBERTHREADS_PTHREAD_INTERNAL_H 1

#include "../include/emft-pthread.h"

struct emfiberthreads_pthread_t {
    struct emfiber_list_t list;
    emscripten_fiber_t fiber;
    void *stack, *asyncifyStack;

    unsigned char exited;
    void *ret;
    emfiber_pthread_t joined;
};

/* Initialize fiberthreads. */
int emfiberthreads_init(void);
#define EMFT_INIT() do { \
    int ret = emfiberthreads_init(); \
    if (ret != 0) \
        return ret; \
} while (0)

/* Go to sleep on a threadlist. */
int emfiberthreads_sleep(emfiber_pthread_t *);

/* Wake a sleeping threadlist. */
int emfiberthreads_wake(emfiber_pthread_t *);

/* Wake only one thread from a sleeping threadlist. */
int emfiberthreads_wake_one(emfiber_pthread_t *);

/* The main fiber, set when the threading system initializes. */
extern struct emfiberthreads_pthread_t emfiberthreads_mainFiber;

/* The current fiber. */
extern emfiber_pthread_t emfiberthreads_self;

/* The *next* fiber, used as part of context switching. */
extern emfiber_pthread_t emfiberthreads_next;

#endif
