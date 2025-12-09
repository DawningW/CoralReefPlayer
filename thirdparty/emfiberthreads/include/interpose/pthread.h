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

#ifndef EMFIBERTHREADS_PTHREAD_H
#define EMFIBERTHREADS_PTHREAD_H 1

/* These __DEFINED_ macros are to override musl. */
#define __DEFINED_pthread_t 1
#define __DEFINED_pthread_mutex_t 1
#define __DEFINED_pthread_once_t 1
#define __DEFINED_pthread_cond_t 1
#define __DEFINED_pthread_attr_t 1
#define __DEFINED_pthread_mutexattr_t 1
#define __DEFINED_pthread_condattr_t 1

#include "emft-pthread.h"

typedef emfiber_pthread_t pthread_t;
typedef emfiber_pthread_mutex_t pthread_mutex_t;
typedef emfiber_pthread_once_t pthread_once_t;
typedef emfiber_pthread_cond_t pthread_cond_t;
typedef emfiber_pthread_attr_t pthread_attr_t;
typedef emfiber_pthread_mutexattr_t pthread_mutexattr_t;
typedef emfiber_pthread_condattr_t pthread_condattr_t;

#define pthread_create emfiber_pthread_create
#define pthread_join emfiber_pthread_join
#define pthread_exit emfiber_pthread_exit
#define pthread_self emfiber_pthread_self
#define pthread_yield emfiber_pthread_yield
#define pthread_mutex_init emfiber_pthread_mutex_init
#define pthread_mutex_destroy emfiber_pthread_mutex_destroy
#define pthread_mutex_lock emfiber_pthread_mutex_lock
#define pthread_mutex_trylock emfiber_pthread_mutex_trylock
#define pthread_mutex_unlock emfiber_pthread_mutex_unlock
#define pthread_once emfiber_pthread_once
#define pthread_cond_init emfiber_pthread_cond_init
#define pthread_cond_destroy emfiber_pthread_cond_destroy
#define pthread_cond_broadcast emfiber_pthread_cond_broadcast
#define pthread_cond_signal emfiber_pthread_cond_signal
#define pthread_cond_wait emfiber_pthread_cond_wait
#define pthread_cond_timedwait emfiber_pthread_cond_timedwait

#define PTHREAD_MUTEX_INITIALIZER EMFIBER_PTHREAD_MUTEX_INITIALIZER
#define PTHREAD_ONCE_INIT EMFIBER_PTHREAD_ONCE_INIT
#define PTHREAD_COND_INITIALIZER EMFIBER_PTHREAD_COND_INITIALIZER

#endif
