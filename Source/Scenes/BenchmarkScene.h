
#pragma once

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
    float m_simTime;
};


