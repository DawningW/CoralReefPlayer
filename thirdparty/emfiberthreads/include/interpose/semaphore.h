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

#ifndef EMFIBERTHREADS_SEMAPHORE_H
#define EMFIBERTHREADS_SEMAPHORE_H 1

#include "emft-semaphore.h"

typedef emfiber_sem_t sem_t;

#define sem_init emfiber_sem_init
#define sem_wait emfiber_sem_wait
#define sem_trywait emfiber_sem_trywait
#define sem_post emfiber_sem_post
#define sem_getvalue emfiber_sem_getvalue
#define sem_destroy emfiber_sem_destroy

#endif
