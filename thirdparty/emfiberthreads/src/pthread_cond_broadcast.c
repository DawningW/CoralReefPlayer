#include "pthread-internal.h"

int emfiber_pthread_cond_broadcast(emfiber_pthread_cond_t *cond) {
    return emfiberthreads_wake(&cond->waiters);
}
