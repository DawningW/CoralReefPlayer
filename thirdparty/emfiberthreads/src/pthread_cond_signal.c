#include "pthread-internal.h"

int emfiber_pthread_cond_signal(emfiber_pthread_cond_t *cond) {
    return emfiberthreads_wake_one(&cond->waiters);
}
