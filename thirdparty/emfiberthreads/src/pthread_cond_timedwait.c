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

void emfiberthreads_timeout_expiry(emfiber_pthread_cond_t *cond, emfiber_pthread_t thr) {
    /* Remove this from waiters. */
    if (cond->waiters == thr)
        cond->waiters = thr->list.next;
    if (thr->list.prev)
        thr->list.prev->list.next = thr->list.next;
    if (thr->list.next)
        thr->list.next->list.prev = thr->list.prev;

    /* Re-add it to the main thread list. */
    thr->list.prev = emfiberthreads_self;
    thr->list.next = emfiberthreads_self->list.next;
    emfiberthreads_self->list.next->list.prev = thr;
    emfiberthreads_self->list.next = thr;
}

EM_JS(static double, emfiberthreads_pthread_mutex_timedwait_setTimeout, (
    void *cond, void *thr, int *timeoutReached, long sec, int nsec
), {
    Module.HEAPU32[timeoutReached/4] = 0;
    var wait = (sec * 1000 + nsec / 1000000) - new Date().getTime();
    return setTimeout(function() {
        Module.HEAPU32[timeoutReached/4] = 1;
        Module.ccall(
            "emfiberthreads_timeout_expiry",
            null, ["number", "number"],
            [cond, thr]
        );
    }, wait);
});

EM_JS(static void, emfiberthreads_pthread_mutex_timedwait_clearTimeout, (
    double timeout
), {
    clearTimeout(timeout);
});

int emfiber_pthread_cond_timedwait(
    emfiber_pthread_cond_t *cond, emfiber_pthread_mutex_t *mutex, const struct timespec *abstime
) {
    int ret;
    int timeoutReached;
    double timeout;

    ret = emfiber_pthread_mutex_unlock(mutex);
    if (ret != 0)
        return ret;

    timeout = emfiberthreads_pthread_mutex_timedwait_setTimeout(
        (void *) cond, (void *) emfiberthreads_self, &timeoutReached,
        abstime->tv_sec, abstime->tv_nsec
    );

    ret = emfiberthreads_sleep(&cond->waiters);
    if (ret != 0)
        return ret;

    if (!timeoutReached)
        emfiberthreads_pthread_mutex_timedwait_clearTimeout(timeout);

    ret = emfiber_pthread_mutex_lock(mutex);
    if (ret != 0)
        return ret;

    return timeoutReached ? ETIMEDOUT : 0;
}
