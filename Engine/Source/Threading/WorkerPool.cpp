
#include "WorkerPool.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    WorkerPool::WorkerPool(unsigned _threadCount)
    {
        // default to 1 if invalid or less than 1
        const unsigned count = _threadCount > 0 ? _threadCount : 1;

        m_workers.reserve(count);
        for (unsigned i = 0; i < count; ++i)
        {
            m_workers.emplace_back([this](std::stop_token _stop)
            {
                WorkerLoop(_stop);
            });
        }
    }

    WorkerPool::~WorkerPool()
    {
        // Request all to stop before
        for (auto& worker : m_workers)
        {
            worker.request_stop();
        }
        m_signal.notify_all();
    }

    // Submits job to be handled by threads
    void WorkerPool::Submit(Job _job)
    {
        {
            std::lock_guard lock (m_mutex);
            m_jobs.push_back(std::move(_job));
        }

        // prompts one worker to start
        m_signal.notify_one();
    }

    // Active logical cores
    unsigned WorkerPool::SuggestThreads()
    {
        const unsigned cores = std::thread::hardware_concurrency();

        if (cores > 3)
        {
            // headroom for main thread and GL driver
            return cores - 2;
        }

        return 1;
    }

    unsigned WorkerPool::GetThreadCount() const
    {
        // size of workers for thread count
        return static_cast<unsigned>(m_workers.size());
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    void WorkerPool::WorkerLoop(std::stop_token _stop)
    {
        // keeps threads running
        while (true)
        {
            Job job;
            {
                // ok so this ONLY locks if the SIGNAL is not waiting
                // which look deceiving because it looks like the mutex stays
                // locked until a notification comes
                std::unique_lock lock(m_mutex);

                // parked until a job arrives or stop requested
                if (!m_signal.wait(lock, _stop, [this] {
                        return !m_jobs.empty();
                    }))
                {
                    // stop while idle
                    return;
                }

                // stopped with job queued, discard
                if (_stop.stop_requested()) return;

                job = std::move(m_jobs.front());
                m_jobs.pop_front();
            }

            // Run outside lock
            job();
        }
    }
}
