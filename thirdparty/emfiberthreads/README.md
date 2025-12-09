# emfiberthreads

libemfiberthreads is an implementation of the pthreads API on top of
[Emscripten's fibers](https://emscripten.org/docs/api_reference/fiber.h.html).

Fibers aren't really threads, so this is very much just a compatibility layer,
not a correct implementation of pthreads. In particular, fibers use cooperative
multitasking. The main result of this is that any code that depends on racey
behavior will not work. There are surprisingly few other results, as any
thread-blocking behavior becomes thread yielding, so most threaded code will
simply run sequentially with libemfiberthreads.

To use libemfiberthreads, just build it and install it to some prefix, then put
that prefix's include directory as an `-I` flag, and link against
`-lemfiberthreads`. Note that libemfiberthreads installs a `pthread.h`, so its
pthreads will be found in place of the real pthreads, but you will still need to
direct your software to use `-lemfiberthreads` instead of `-pthread`.

By default, fiber pseudo-threads will have a 64KB stack and 4KB asyncify stack.
This can be overridden with a `make` option: `make STACK_SIZE=131072
ASYNCIFY_STACK_SIZE=65536`. It is not configurable at runtime.


## Licensing

libemfiberthreads is released under the so-called “0-clause BSD license”. No
attribution is required.


## How well does it work?

libemfiberthreads lives by the motto “good enough is the enemy of just barely
adequate”. I've only implemented exactly enough functionality to compile what I
need (namely, [libav.js](https://github.com/Yahweasel/libav.js/)).

Cooperative multitasking is not a substitute for preemption, but in reality,
most threaded software doesn't actually need preemption. Without preemption, all
blocking calls (e.g. trying to take a lock that's already held) simply yield to
another fiber. Things will only go wrong if a user attempts to observe
concurrent behavior, such as trylocking in a loop or checking a variable in a
loop without yielding. Most software doesn't behave this way, so works fine.


## How does it work?

“Fibers” is just one of a million names for stackful coroutines, or green
threads, or contexts. Each fiber is a thread of execution; the only thing that
distinguishes them from threads is that they aren't scheduled by the OS, and so
can't be scheduled at the same time as other fibers.

This means that it's up to the fibers themselves to do the scheduling. This is
accomplished by cooperative multitasking: fibers in libemfiberthreads
voluntarily give up control to the next fiber.

This is accomplished by keeping the thread structure as a circular linked list.
Every time a thread calls `pthread_yield` (including all the times it's
implicitly called to block, such as in `pthread_mutex_lock`), the next thread in
the list is switched to. When threads are woken up (such as in
`pthread_mutex_unlock`), they're added to the linked list of the current thread,
i.e., the list of running threads.

As a small detail, some other headers are also overridden just to prevent them
from loading in the original `pthread_t`, which conflicts with
libemfiberthreads's definition.
