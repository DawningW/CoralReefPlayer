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

#ifndef EMFIBERTHREADS_EMFT_SEMAPHORE_H
#define EMFIBERTHREADS_EMFT_SEMAPHORE_H 1

#include "emft-pthread.h"

/**
 * emfiberthreads semaphore.
 */
typedef struct emfiber_sem_t {
    /**
     * Threads waiting on the semaphore.
     */
    emfiber_pthread_t waiters;

    /**
     * Current value of the semaphore.
     */
    unsigned int value;
} emfiber_sem_t;

int emfiber_sem_init(emfiber_sem_t *, int, unsigned int);
int emfiber_sem_wait(emfiber_sem_t *);
int emfiber_sem_trywait(emfiber_sem_t *);
int emfiber_sem_post(emfiber_sem_t *);
static inline int emfiber_sem_getvalue(emfiber_sem_t *sem, int *value) { *value = (int) sem->value; return 0; }
static inline int emfiber_sem_destroy(emfiber_sem_t *sem) { (void) sem; return 0; }

#endif
