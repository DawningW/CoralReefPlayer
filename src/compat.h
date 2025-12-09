#pragma once

#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)

#include <thread>
#include <atomic>

using CRPThread = std::thread;
using CRPFlag = std::atomic_flag;

#else

#include <functional>
#include <stdexcept>
#include <emscripten.h>
extern "C"
{
#include "emft-pthread.h"
}

class CRPThread
{
public:
    typedef emfiber_pthread_t native_handle_type;

    CRPThread() noexcept : m_handle(nullptr) {}

    template <class F, class ...Args> 
    explicit CRPThread(F&& f, Args&&... args)
    {
        auto entry = new std::function<void()>;
        *entry = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        emfiber_pthread_create(&m_handle, nullptr, [](void* arg) -> void*
        {
            auto entry = static_cast<std::function<void()>*>(arg);
            (*entry)();
            delete entry;
            return nullptr;
        }, entry);
    }

    ~CRPThread()
    {
        if (joinable())
        {
            std::terminate();
        }
    }

    CRPThread(const CRPThread&) = delete;
    CRPThread(CRPThread&& t) noexcept : m_handle(t.m_handle)
    {
        t.m_handle = nullptr;
    }

    CRPThread& operator=(const CRPThread&) = delete;
    CRPThread& operator=(CRPThread&& t) noexcept
    {
        if (joinable())
        {
            std::terminate();
        }
        m_handle = t.m_handle;
        t.m_handle = nullptr;
        return *this;
    }

    void swap(CRPThread& t) noexcept
    {
        std::swap(m_handle, t.m_handle);
    }

    bool joinable() const noexcept
    {
        return m_handle != nullptr;
    }

    void join()
    {
        if (!joinable())
        {
            throw std::runtime_error("Thread not joinable");
        }
        emfiber_pthread_join(m_handle, nullptr);
        m_handle = nullptr;
    }

    void detach()
    {
        if (!joinable())
        {
            throw std::runtime_error("Thread not joinable");
        }
        m_handle = nullptr;
    }

    native_handle_type native_handle()
    {
        return m_handle;
    }

private:
    native_handle_type m_handle;
};

struct CRPFlag
{
    CRPFlag() noexcept : m_state(false)
    {
        emfiber_pthread_mutex_init(&m_mutex, nullptr);
        emfiber_pthread_cond_init(&m_cond, nullptr);
    }

    CRPFlag(const CRPFlag&) = delete;
    CRPFlag& operator=(const CRPFlag&) = delete;

    bool test() noexcept
    {
        emfiber_pthread_mutex_lock(&m_mutex);
        bool ret = m_state;
        emfiber_pthread_mutex_unlock(&m_mutex);
        return ret;
    }

    bool test_and_set() noexcept
    {
        emfiber_pthread_mutex_lock(&m_mutex);
        bool ret = m_state;
        m_state = true;
        emfiber_pthread_mutex_unlock(&m_mutex);
        return ret;
    }

    void clear() noexcept
    {
        emfiber_pthread_mutex_lock(&m_mutex);
        m_state = false;
        emfiber_pthread_mutex_unlock(&m_mutex);
        if (!m_state) notify_all();
    }

    void wait(bool old) const noexcept
    {
        emfiber_pthread_mutex_lock(&m_mutex);
        while (m_state == old)
        {
            emfiber_pthread_cond_wait(&m_cond, &m_mutex);
        }
        emfiber_pthread_mutex_unlock(&m_mutex);
    }

    void notify_one() noexcept
    {
        emfiber_pthread_cond_signal(&m_cond);
    }

    void notify_all() noexcept
    {
        emfiber_pthread_cond_broadcast(&m_cond);
    }

private:
    bool m_state;
    mutable emfiber_pthread_mutex_t m_mutex;
    mutable emfiber_pthread_cond_t m_cond;
};

#undef ATOMIC_FLAG_INIT
#define ATOMIC_FLAG_INIT CRPFlag()

#endif
