
#include "WorkerPool.h"
#include "Helpers/Printer.hpp"

#include <algorithm>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

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

    // Physical core layout via the OS info
    WorkerPool::CpuTopology WorkerPool::QueryTopology()
    {
        CpuTopology cpuTopol;
        cpuTopol.logical = std::thread::hardware_concurrency();

#ifdef _WIN32
        DWORD bytes = 0;
        GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bytes);

        // no info, return total logical threads
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytes == 0)
        {
            return cpuTopol;
        }


        std::vector<char> buffer(bytes);
        auto* head = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data());
        // no avail info for logic cpu, return
        if (!GetLogicalProcessorInformationEx(RelationProcessorCore, head, &bytes))
        {
            return cpuTopol;
        }

        // ONE entry per each physical core, look if its a P or E core, and if it has HT/SMT
        BYTE maxClass = 0;
        std::vector<BYTE> coreClasses;

        // interate over each core
        for (DWORD offset = 0; offset < bytes;)
        {
            auto* info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(
                buffer.data() + offset);

            if (info->Relationship == RelationProcessorCore)
            {
                ++cpuTopol.physical;

                // if has HT/SMT, add to count
                if (info->Processor.Flags & LTP_PC_SMT)
                {
                    ++cpuTopol.smtCores;
                }

                // first byte of core info is CORE TYPE (intel e or p cores)
                // classified in classes, save them, sort later
                const BYTE coreClass = reinterpret_cast<const BYTE*>(&info->Processor)[1];

                coreClasses.push_back(coreClass);
                maxClass = std::max(maxClass, coreClass);
            }
            offset += info->Size;
        }

        // no cores?
        if (cpuTopol.physical == 0) return cpuTopol;

        // Hybrid cpu, use previously computed core class and sort cores in P cores and E cores
        // Higher class means P core, lower E core
        for (const BYTE coreClass : coreClasses)
        {
            if (coreClass == maxClass)
                ++cpuTopol.pCores;
            else
                ++cpuTopol.eCores;
        }

        // set hybrid if e cores more than 1
        cpuTopol.hybrid = cpuTopol.eCores > 0;
        cpuTopol.valid  = true;

#endif
        return cpuTopol;
    }

    // cpu type aware working count
    unsigned WorkerPool::SuggestThreads()
    {
        const CpuTopology cpuTopol = QueryTopology();

        // no topology, hard fall back
        if (!cpuTopol.valid)
        {
            // if thread count 4 or more, remove 2
            if (cpuTopol.logical > 3)
            {
                return cpuTopol.logical - 2;
            }

            return 1;
        }

        // P cores (and non classified efficency cores) share Pcore value for amnount of workers
        const int pCores  = static_cast<int>(cpuTopol.pCores);

        int workers;

        // physical not threads
        // set base workers, if less than 5 cores only scale off 1
        if (pCores <= 4 )
        {
            workers = pCores - 1;
        }
        else
        {
            // 4 or more, scale off 2 from total
            workers = pCores - 2;
        }

        if (cpuTopol.hybrid)
        {
            // all E cores join, minus one kept free for OS background services
            workers += static_cast<int>(cpuTopol.eCores) - 1;
        }

        const unsigned suggested = static_cast<unsigned>(std::max(workers, 1));

        // log configuration
        InfoLog("[MT POOL] CPU topology: ", cpuTopol.physical, " physical (", cpuTopol.pCores, "P/",
                cpuTopol.eCores, "E, ", cpuTopol.smtCores, " SMT) / ", cpuTopol.logical,
                " logical. Suggested worker count is: '", suggested,"'");

        return suggested;
    }

    // Sizes queue based on pCore and eCore settings
    // avoid stalling the main consegution with too many per frame
    unsigned WorkerPool::SuggestInFlightCap(float _perPWorker, float _perEWorker)
    {
        // pWorker is P cores on intel cpu and normal cores on homogenous cpu
        // eWorker is E cores on intel, you might want less work these small bois
        const float perPWorker = std::max(_perPWorker, 0.5f);
        const float perEWorker = std::max(_perEWorker, 0.5f);

        const CpuTopology cpuTopol = QueryTopology();

        // No topology got, fallback to legacy logical cores - 2
        // this should not happen, if it does you might want to set specific values
        if (!cpuTopol.valid)
        {
            Warn("[MT POOL] Cpu topology failed to get, you might wanna set worker count manually!");

            const int fallback = static_cast<int>(static_cast<float>(cpuTopol.logical / 2u) * perPWorker);
            return static_cast<unsigned>(std::max(fallback, 8));
        }

        int cap = 0;

        if (cpuTopol.hybrid)
        {
            const int pCores = static_cast<int>(cpuTopol.pCores);

            // on hybrid cpus devided total workers per core type
            const int pHeadroom = pCores > 4 ? 2 : 1;
            const int pWorkers  = static_cast<int>(static_cast<float>(pCores - pHeadroom) * perPWorker);
            const int eWorkers  = static_cast<int>(static_cast<float>(static_cast<int>(cpuTopol.eCores) - 1) * perEWorker);

            cap = pWorkers + eWorkers;
        }
        else
        {
            // if homogenous cpu, all cores same
            const int physical = static_cast<int>(cpuTopol.physical);

            const int headroom = physical <= 4 ? 1 : 2;
            cap = static_cast<int>(static_cast<float>(physical - headroom) * perPWorker);
        }

        return static_cast<unsigned>(std::max(cap, 4));
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
