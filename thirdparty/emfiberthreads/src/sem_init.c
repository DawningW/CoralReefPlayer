#include "pthread-internal.h"
#include "../include/emft-semaphore.h"

int emfiber_sem_init(emfiber_sem_t *sem, int shared, unsigned int value) {
    (void) shared;
    sem->waiters = NULL;
    sem->value = value;
    return 0;
}
