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

int emfiberthreads_sleep(emfiber_pthread_t *head) {
    EMFT_INIT();
    emfiberthreads_next = emfiberthreads_self->list.next;
    if (*head)
        (*head)->list.prev = emfiberthreads_self;
    emfiberthreads_self->list.prev->list.next = emfiberthreads_self->list.next;
    emfiberthreads_self->list.next->list.prev = emfiberthreads_self->list.prev;
    emfiberthreads_self->list.prev = NULL;
    emfiberthreads_self->list.next = *head;
    *head = emfiberthreads_self;
    emfiber_pthread_yield();
    return 0;
}
