
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

        // Physical CPU layout
        // take in count homogenous and hybrid cpu layout
        // thanks intel for making this horrendously complicated, much love
        struct CpuTopology
        {
            unsigned logical  = 0; // threads count
            unsigned physical = 0; // physical cpu core count
            unsigned smtCores = 0; // physical cpu cores with HT
            unsigned pCores   = 0; // P cores (intel) if same as physical homogeneous
            unsigned eCores   = 0; // E cores (intel) if 0 homogeneous
            bool     hybrid   = false;
            bool     valid    = false;
        };

        static CpuTopology QueryTopology();
        static unsigned SuggestThreads();

        // returns suggested cap using json p and e core settings
        static unsigned SuggestInFlightCap(float _perPWorker = 4.0f, float _perEWorker = 1.0f);

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
