
#pragma once
#include <vector>

#include "ISubSystem.h"
#include "Helpers/TypeInfo.h"
#include "Benchmark/FrameStats.h"

namespace RR
{
    class BenchmarkSubSystem : public ISubSystem
    {
    public:
        SUBSYSTEM(BenchmarkSubSystem, RR::ISubSystem)

        BenchmarkSubSystem();
        ~BenchmarkSubSystem() override;

    protected:
        bool Init() override;
        void Update(float _deltaTime) override;
        void Destroy() override;

    private:
        std::vector<FrameSample> m_samples;
        bool m_logging = false;
    };
}
