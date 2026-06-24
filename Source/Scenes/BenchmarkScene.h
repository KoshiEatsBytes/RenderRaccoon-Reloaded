
#pragma once

#include "Base/VoxelScene.h"
#include "Benchmark/BenchmarkSubSystem.h"
#include "WorldGen/WorldGenConfig.h"

class BenchmarkScene : public VoxelScene
{
public:
    explicit BenchmarkScene(const RR::RunInfo& _runInfo, const WORLDGEN::WorldGenConfig& _config);
    explicit BenchmarkScene(const RR::RunInfo& _runInfo);
    ~BenchmarkScene() override;

protected:
    void OnInit() override;
    void OnUpdate(float _deltaTime) override;

    void OnPauseEnter() override;
    void OnPausePrimary() override;

private:
    void LoadNextScene();

    // PLACEHOLDER
    RR::BenchmarkSubSystem* m_bench = nullptr;
    int   m_discard = 0;
    float m_yaw     = 0.f;
    float m_timer   = 0.0f;
    bool  m_fired   = false;
};


