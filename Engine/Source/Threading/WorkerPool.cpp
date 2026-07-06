
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

        int workers = 0;

        if (cpuTopol.hybrid)
        {
            // always include all E cores into total count
            workers = static_cast<int>(cpuTopol.eCores);

            // P core contribution with headroom for main thread and gl
            const int pCores = static_cast<int>(cpuTopol.pCores);

            // if more than 4 pcores, leave 2 empty
            if (pCores > 4)
            {
                workers += pCores - 2;
            }
            else
            {
                // if less than 4 or 4 pcroes, 1 empty
                workers += std::max(pCores - 1, 0);
            }
        }
        else if (cpuTopol.physical <= 4)
        {

            const int physical = static_cast<int>(cpuTopol.physical);

            // if quad core or less, check if has smt, if so worker logic thread - 2
            // otherwise logic threads - 1
            if (cpuTopol.smtCores > 0)
            {
                workers = physical - 1;
            }
            else
            {
                workers = physical - 2;
            }
        }
        else
        {
            // if more than 4 cores always physical - 2
            workers = static_cast<int>(cpuTopol.physical) - 2;
        }

        const unsigned suggested = static_cast<unsigned>(std::max(workers, 1));

        InfoLog("[MT POOL] CPU topology: ", cpuTopol.physical, " physical (", cpuTopol.pCores, "P/",
                cpuTopol.eCores, "E, ", cpuTopol.smtCores, " SMT) / ", cpuTopol.logical,
                " logical. Suggested worker count is: '", suggested,"'");

        return suggested;
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
