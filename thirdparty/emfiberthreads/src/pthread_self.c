#include "pthread-internal.h"

emfiber_pthread_t emfiber_pthread_self() {
    return emfiberthreads_self;
}
