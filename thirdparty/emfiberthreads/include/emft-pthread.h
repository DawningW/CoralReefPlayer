/*
 * Copyright (C) 2024, 2025 Yahweasel and contributors
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

#ifndef EMFIBERTHREADS_EMFT_PTHREAD_H
#define EMFIBERTHREADS_EMFT_PTHREAD_H 1

#include <emscripten/fiber.h>

/* Sufficient pthread-like API on top of Emscripten fibers */

struct emfiberthreads_pthread_t;

/**
 * emfiberthread pthreads are allocated structures.
 */
typedef struct emfiberthreads_pthread_t *emfiber_pthread_t;

/**
 * To rotate fibers, we keep them on a list.
 */
struct emfiber_list_t {
    emfiber_pthread_t prev, next;
};

typedef struct emfiber_pthread_mutex_t {
    /**
     * Threads currently waiting for the lock. Linear linked list.
     */
    emfiber_pthread_t waiters;

    /**
     * Thread currently holding the lock, or NULL for none. Not a list head.
     */
    emfiber_pthread_t holder;
} emfiber_pthread_mutex_t;

typedef unsigned char emfiber_pthread_once_t;

typedef struct emfiber_pthread_cond_t {
    /**
     * Threads waiting on this condition, linear linked list.
     */
    emfiber_pthread_t waiters;
} emfiber_pthread_cond_t;

typedef int emfiber_pthread_attr_t;
typedef int emfiber_pthread_mutexattr_t;
typedef int emfiber_pthread_condattr_t;

#define EMFIBER_PTHREAD_MUTEX_INITIALIZER { NULL, NULL }
#define EMFIBER_PTHREAD_ONCE_INIT 0
#define EMFIBER_PTHREAD_COND_INITIALIZER { NULL }

#include <time.h>

/* We rename functions to avoid clashes with stub libraries. */
int emfiber_pthread_create(emfiber_pthread_t *, const emfiber_pthread_attr_t *, void *(*)(void *), void *);
int emfiber_pthread_join(emfiber_pthread_t, void **);
void emfiber_pthread_exit(void *);
emfiber_pthread_t emfiber_pthread_self(void);
int emfiber_pthread_yield(void);

int emfiber_pthread_mutex_init(emfiber_pthread_mutex_t *, const emfiber_pthread_mutexattr_t *);
static inline int emfiber_pthread_mutex_destroy(emfiber_pthread_mutex_t *mutex) { (void) mutex; return 0; }
int emfiber_pthread_mutex_lock(emfiber_pthread_mutex_t *);
int emfiber_pthread_mutex_trylock(emfiber_pthread_mutex_t *);
int emfiber_pthread_mutex_unlock(emfiber_pthread_mutex_t *);

int emfiber_pthread_once(emfiber_pthread_once_t *, void (*)(void));

int emfiber_pthread_cond_init(emfiber_pthread_cond_t *, const emfiber_pthread_condattr_t *);
static inline int emfiber_pthread_cond_destroy(emfiber_pthread_cond_t *cond) { (void) cond; return 0; }
int emfiber_pthread_cond_broadcast(emfiber_pthread_cond_t *);
int emfiber_pthread_cond_signal(emfiber_pthread_cond_t *);
int emfiber_pthread_cond_wait(emfiber_pthread_cond_t *, emfiber_pthread_mutex_t *);
int emfiber_pthread_cond_timedwait(emfiber_pthread_cond_t *, emfiber_pthread_mutex_t *, const struct timespec *);

#endif
