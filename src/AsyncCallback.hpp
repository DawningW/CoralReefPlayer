#pragma once

#include <functional>
#include <atomic>
#include <thread>

template <typename... Args>
class AsyncCallback
{
public:
    using Callback = std::function<void(Args...)>;

    AsyncCallback() : stop(true) {}

    template <typename T>
    AsyncCallback(T&& callback) : callback(std::forward<T>(callback)), stop(true)
    {
        if (this->callback)
            thread = std::thread(&AsyncCallback::loop, this);
    }

    ~AsyncCallback()
    {
        finish();
    }

    template <typename T>
    AsyncCallback& operator=(T&& callback)
    {
        finish();
        this->callback = std::forward<T>(callback);
        if (this->callback)
            thread = std::thread(&AsyncCallback::loop, this);
        return *this;
    }

    template <typename... T>
    void invoke(T&&... args)
    {
        if (!callback)
            return;
        next = std::bind(callback, std::forward<T>(args)...);
        signal.test_and_set();
        signal.notify_all();
    }

    template <typename... T>
    void invokeSync(T&&... args)
    {
        if (!callback)
            return;
        callback(std::forward<T>(args)...);
    }

    template <typename... T>
    void operator()(T&&... args)
    {
        invoke(std::forward<T>(args)...);
    }

    void wait()
    {
        if (stop)
            return;
        signal.wait(true);
    }

private:
    void loop()
    {
        stop = false;
        while (!stop)
        {
            signal.wait(false);
            if (!stop)
                next();
            signal.clear();
            signal.notify_all();
        }
    }

    void finish()
    {
        if (stop)
            return;
        stop = true;
        signal.test_and_set();
        signal.notify_all();
        thread.join();
        thread = std::thread();
    }

private:
    Callback callback;

    volatile bool stop;
    std::thread thread;
    std::atomic_flag signal = ATOMIC_FLAG_INIT; // For Android compatibility
    std::function<void()> next;
};
