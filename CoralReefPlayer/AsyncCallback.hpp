#pragma once

#include <functional>
#include <atomic>
#include <thread>

template<typename... Args>
class AsyncCallback
{
public:
    using Func = void(Args...);

    AsyncCallback() : stop(true) {}

    AsyncCallback(std::function<Func> callback) : AsyncCallback(), callback(callback)
    {
        if (callback)
            thread = std::thread(&AsyncCallback::loop, this);
    }

    AsyncCallback& operator=(std::function<Func> callback)
    {
        finish();
        this->callback = callback;
        if (callback)
            thread = std::thread(&AsyncCallback::loop, this);
        return *this;
    }

    ~AsyncCallback()
    {
        finish();
    }

    void invoke(Args&&... args)
    {
        if (!callback)
            return;
        next = std::bind(callback, std::forward<Args>(args)...);
        signal.test_and_set();
        signal.notify_all();
    }

    void operator()(Args&&... args)
    {
        invoke(std::forward<Args>(args)...);
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
    std::function<Func> callback;

    volatile bool stop;
    std::thread thread;
    std::atomic_flag signal;
    std::function<void()> next;
};
