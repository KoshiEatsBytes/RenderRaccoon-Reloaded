
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
    void OnGui() override;

    void OnPauseEnter() override;
    void OnPausePrimary() override;
    void OnPauseSecondary() override;

private:
    void AbortRun();

    void LoadNextScene();
    void ApplyCameraSample(const BENCH::CameraSample& _sample);

    // Camera and benchmark
    RR::BenchmarkSubSystem* m_bench = nullptr;
    BENCH::CameraPath m_path;

    // Limits for loading failsafe
    // calculate average time remaining using loading progress
    // timeout and its slack are json knobs, defaults used if missing
    float m_maxWarmUpSeconds = 300.0f;
    float m_warmUpHeadroom   = 1.25f;
    static constexpr float kStuckSeconds      = 30.0f;
    static constexpr float kPaceGraceSeconds  = 30.0f;
    static constexpr float kAbortFrameSeconds = 0.25f;
    static constexpr int   kAbortSlowFrames   = 4;
    int m_slowFrames = 0;

    // log details
    static constexpr int   kDiscardFrames = 2;
    static constexpr float kMaxDeltaTime  = 1.0f / 20.0f;
    bool  m_pathComplete = false;
    float m_simTime      = 0.0f;

    // Warm up
    std::chrono::steady_clock::time_point m_warmUpStart;
    bool  m_warmUpTimerStarted = false;
    bool  m_warmedUp           = false;
    float m_warmUpSeconds      = 0.0f;

    // Stuck watchdog
    std::chrono::steady_clock::time_point m_lastProgressTime;
    float m_lastCoverage = 0.0f;
};


