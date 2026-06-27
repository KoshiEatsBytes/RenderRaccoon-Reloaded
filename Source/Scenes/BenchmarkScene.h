
#pragma once
#include <chrono>

#include "Base/VoxelScene.h"
#include "Benchmark/BenchmarkSubSystem.h"
#include "Benchmark/CameraPath.hpp"
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
    void OnPauseSecondary() override;

private:
    void LoadNextScene();
    void ApplyCameraSample(const BENCH::CameraSample& _sample);

    // Camera and benchmark
    RR::BenchmarkSubSystem* m_bench = nullptr;
    BENCH::CameraPath m_path;

    static constexpr int   kDiscardFrames = 2;
    static constexpr float kMaxDeltaTime  = 1.0f / 20.0f;
    bool  m_pathComplete = false;
    float m_simTime      = 0.0f;

    // Warm up
    std::chrono::steady_clock::time_point m_warmUpStart;
    bool  m_warmUpTimerStarted = false;
    bool  m_warmedUp           = false;
    float m_warmUpSeconds      = 0.0f;
};


