#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>
#include <list>

class ThreadPool {
public:
    ThreadPool() = default;
    ~ThreadPool() { join_all(); }

    void add_thread(std::thread t) {
        m_threads.push_back( std::move(t) );
    }

    template<class F>
    void create_thread(F&& f) {
        m_threads.push_back( std::thread(std::forward<F>(f)) );
    }

    void join_all() {
        for (auto &t : m_threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

protected:
    std::list<std::thread> m_threads;
};

#endif // THREAD_POOL_H
