#ifdef __OHOS__
extern "C" {

#include <errno.h>
#include <pthread.h>

int* __errno(void) {
    return __errno_location();
}

int __register_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void), void* dso) {
    (void)(dso);
    return pthread_atfork(prepare, parent, child);
}

}
#endif
