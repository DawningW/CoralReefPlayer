CC=emcc
CFLAGS=-Oz -Wno-undefined-internal
AR=emar
RANLIB=emranlib
PREFIX=/usr
STACK_SIZE=65536
ASYNCIFY_STACK_SIZE=4096

OBJ=\
	obj/pthread_create.o \
	obj/pthread_join.o \
	obj/pthread_exit.o \
	obj/pthread_self.o \
	obj/pthread_yield.o \
	obj/pthread_mutex_init.o \
	obj/pthread_mutex_lock.o \
	obj/pthread_mutex_trylock.o \
	obj/pthread_mutex_unlock.o \
	obj/pthread_once.o \
	obj/pthread_cond_init.o \
	obj/pthread_cond_broadcast.o \
	obj/pthread_cond_signal.o \
	obj/pthread_cond_wait.o \
	obj/pthread_cond_timedwait.o \
	obj/sem_init.o \
	obj/sem_wait.o \
	obj/sem_trywait.o \
	obj/sem_post.o \
	obj/emfiberthreads_init.o \
	obj/emfiberthreads_sleep.o \
	obj/emfiberthreads_wake.o \
	obj/emfiberthreads_wake_one.o \
	obj/globals.o

all: libemfiberthreads.a

libemfiberthreads.a: $(OBJ)
	$(AR) rc $@ $(OBJ)
	$(RANLIB) $@

obj/%.o: src/%.c
	mkdir -p obj
	$(CC) $(CFLAGS) -DEMFIBERTHREADS_STACK_SIZE=$(STACK_SIZE) \
		-DEMFIBERTHREADS_ASYNCIFY_STACK_SIZE=$(ASYNCIFY_STACK_SIZE) \
		-c $< -o $@

install:
	mkdir -p $(PREFIX)/include
	install -m 0644 include/emft-pthread.h $(PREFIX)/include/emft-pthread.h
	install -m 0644 include/emft-semaphore.h $(PREFIX)/include/emft-semaphore.h
	mkdir -p $(PREFIX)/lib
	install -m 0644 libemfiberthreads.a $(PREFIX)/lib/libemfiberthreads.a

install-interpose: install
	mkdir -p $(PREFIX)/include/sys
	install -m 0644 include/interpose/pthread.h $(PREFIX)/include/pthread.h
	install -m 0644 include/interpose/semaphore.h $(PREFIX)/include/semaphore.h
	install -m 0644 include/interpose/signal.h $(PREFIX)/include/signal.h
	install -m 0644 include/interpose/mqueue.h $(PREFIX)/include/mqueue.h
	install -m 0644 include/interpose/sys/types.h $(PREFIX)/include/sys/types.h

clean:
	rm -rf obj
	rm -f libemfiberthreads.a