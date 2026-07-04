
#pragma once
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace RR
{
    // fixed pool of worker thread consuming a shared queue
    class WorkerPool
    {
    public:
        using Job = std::function<void()>;

        explicit WorkerPool(unsigned _threadCount = SuggestThreads());
        ~WorkerPool();

        // Prevent copying
        WorkerPool(const WorkerPool&) = delete;
        WorkerPool& operator=(const WorkerPool&) = delete;

        void Submit(Job _job);

        static unsigned SuggestThreads();
        unsigned GetThreadCount() const;

    private:
        void WorkerLoop(std::stop_token _stop);

        std::mutex                  m_mutex;
        std::condition_variable_any m_signal;
        std::deque<Job>             m_jobs;

        // hold current processes
        std::vector<std::jthread> m_workers;
    };
}
