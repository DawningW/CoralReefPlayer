cmake_minimum_required(VERSION 3.10)
project(emfiberthreads C)

set(STACK_SIZE 65536 CACHE STRING "Stack size for emfiberthreads")
set(ASYNCIFY_STACK_SIZE 4096 CACHE STRING "Asyncify stack size for emfiberthreads")

add_library(emfiberthreads STATIC
    src/pthread_create.c
    src/pthread_join.c
    src/pthread_exit.c
    src/pthread_self.c
    src/pthread_yield.c
    src/pthread_mutex_init.c
    src/pthread_mutex_lock.c
    src/pthread_mutex_trylock.c
    src/pthread_mutex_unlock.c
    src/pthread_once.c
    src/pthread_cond_init.c
    src/pthread_cond_broadcast.c
    src/pthread_cond_signal.c
    src/pthread_cond_wait.c
    src/pthread_cond_timedwait.c
    src/sem_init.c
    src/sem_wait.c
    src/sem_trywait.c
    src/sem_post.c
    src/emfiberthreads_init.c
    src/emfiberthreads_sleep.c
    src/emfiberthreads_wake.c
    src/emfiberthreads_wake_one.c
    src/globals.c
)

target_include_directories(emfiberthreads INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(emfiberthreads PRIVATE -Oz -Wno-undefined-internal)

target_compile_definitions(emfiberthreads PRIVATE
    EMFIBERTHREADS_STACK_SIZE=${STACK_SIZE}
    EMFIBERTHREADS_ASYNCIFY_STACK_SIZE=${ASYNCIFY_STACK_SIZE}
)

option(EMFIBERTHREADS_INSTALL "Install emfiberthreads" OFF)
if(EMFIBERTHREADS_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS emfiberthreads
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasm32-emscripten
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasm32-emscripten
    )
    install(CODE "execute_process(COMMAND
        ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_INSTALL_LIBDIR}/wasm32-emscripten/libemfiberthreads.a
        ${CMAKE_INSTALL_LIBDIR}/libemfiberthreads.a
    )")
    install(FILES
        include/emft-pthread.h
        include/emft-semaphore.h
        include/interpose/pthread.h
        include/interpose/semaphore.h
        include/interpose/signal.h
        include/interpose/mqueue.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(FILES
        include/interpose/sys/types.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sys
    )
endif()
