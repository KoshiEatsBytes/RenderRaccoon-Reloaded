
#include <algorithm>

#include "BenchmarkScene.h"
#include "MainMenuScene.h"
#include "Benchmark/BenchmarkRunPresets.hpp"
#include "Components/FreeCameraComponent.h"
#include "Voxels/ChunkManager.h"

// PUBLIC --------------------------------------------------------------------------------------------------------------

BenchmarkScene::BenchmarkScene(const RR::RunInfo& _runInfo, const WORLDGEN::WorldGenConfig& _config)
    : VoxelScene(_runInfo, _config)
{
}

BenchmarkScene::BenchmarkScene(const RR::RunInfo& _runInfo)
    : VoxelScene(_runInfo)
{
}

BenchmarkScene::~BenchmarkScene()
= default;

// PROTECTED -----------------------------------------------------------------------------------------------------------

void BenchmarkScene::OnInit()
{
    // Benchmarks are non-resumable from pause
    SetResumable(false);
    SetPrimaryButtonText("RESTART BENCHMARK");
    SetSecondaryButtonText("EXIT TO MAIN MENU");

    // get hold of benchmark
    m_bench = RR::Engine::GetInstance().GetAppManager().GetSubSystem<RR::BenchmarkSubSystem>();

    // Create free cam
    m_cam     = CreateObject("BenchmarkCam");
    m_camComp = m_cam->AddComponent<RR::FreeCameraComponent>();
    m_cam->SetPosition(vec3(0.f, 120.f, 0.f));
    SetMainCamera(m_cam);

    SetCursorEnabled(false);
    m_camComp->SetDiscardInput(true);

    m_path = BENCH::GetCameraPath(BENCH::CAMERA_PATH_ID::DETERMINISTIC);
    ApplyCameraSample(m_path.Sample(0.0f));
}

void BenchmarkScene::OnUpdate(float _deltaTime)
{
    if (m_paused) return;

    if (!m_warmedUp)
    {
        // start load timer of first update
        if (!m_warmUpTimerStarted)
        {
            m_warmUpStart = std::chrono::steady_clock::now();
            m_warmUpTimerStarted = true;
        }

        // hold at spawn until generated
        if (!m_chunkManager->IsStreamingIdle())
        {
            ApplyCameraSample(m_path.Sample(0.0f));
            return;
        }

        // warm up complete, record load time, begin run
        m_warmedUp = true;
        m_warmUpSeconds = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - m_warmUpStart).count();
        // save into runinfo
        m_runInfo.warmUpSeconds = m_warmUpSeconds;

        if (!m_bench)
        {
            RR::Error("[BENCHMARK] Benchmark subsystem not present!");
            return;
        }

        m_bench->RequestStartLogging(m_runInfo, kDiscardFrames);
    }

    m_simTime += std::min(_deltaTime, kMaxDeltaTime);
    ApplyCameraSample(m_path.Sample(m_simTime));

    if (m_bench)
    {
        // ship scene data directly to benchmark
        m_bench->RecordSceneMetrics(
            m_simTime,
            m_cam->GetWorldPosition(),
            m_chunkManager->GetCoverage()
        );
    }

    // Path complete, proceed
    if (!m_pathComplete && m_simTime >= m_path.Duration())
    {
        // log to disk
        if (m_bench) m_bench->RequestStopLogging();

        m_pathComplete = true;
        LoadNextScene();
    }
}

void BenchmarkScene::OnPauseEnter()
{
    VoxelScene::OnPauseEnter();
    // Discard benchmark on pause
    if (m_bench) m_bench->RequestDiscard();
}

void BenchmarkScene::OnPausePrimary()
{
    auto& appMan = RR::Engine::GetInstance().GetAppManager();

    // if deterministic and restart prompted, reload entire sequence
    if (m_runInfo.deterministic)
    {
        using namespace DETERMINISTIC;

        gCurrentSceneStep = static_cast<uInt8>(SCENE::BASELINE);
        appMan.RequestSceneLoad<BenchmarkScene>(GetRunPreset(SCENE::BASELINE));
        return;
    }

    // For custom only, restart this exact scene
    appMan.RequestSceneLoad<BenchmarkScene>(m_runInfo, m_genConfig);
}

void BenchmarkScene::OnPauseSecondary()
{
    // User has quit
    auto& appMan = RR::Engine::GetInstance().GetAppManager();
    appMan.RequestSceneLoad<MainMenuScene>(BENCH_SUCCESS::FAILED);
}

void BenchmarkScene::LoadNextScene()
{
    auto& appMan = RR::Engine::GetInstance().GetAppManager();

    if (!m_runInfo.deterministic)
    {
        appMan.RequestSceneLoad<MainMenuScene>(BENCH_SUCCESS::CUSTOM);
        return;
    }

    using namespace DETERMINISTIC;

    // Hit end of sequence, return to main menu
    if (gCurrentSceneStep + 1 == kSceneCount)
    {
        RR::Success("[DETERMINISTIC BENCHMARK] Deterministic benchmark has concluded successfully");
        appMan.RequestSceneLoad<MainMenuScene>(BENCH_SUCCESS::DETERMINISTIC);
        return;
    }

    // Load next scene in sequence
    gCurrentSceneStep++;
    RR::RunInfo info = GetRunPreset(static_cast<SCENE>(gCurrentSceneStep));
    appMan.RequestSceneLoad<BenchmarkScene>(info);
}

void BenchmarkScene::ApplyCameraSample(const BENCH::CameraSample& _sample)
{
    const quat qYaw   = glm::angleAxis(glm::radians(_sample.yaw), vec3(0.f, 1.f, 0.f));
    const quat qPitch = glm::angleAxis(glm::radians(_sample.pitch), vec3(1.f, 0.f, 0.f));

    m_cam->SetWorldRotation(glm::normalize(qYaw * qPitch));
    m_cam->SetWorldPosition(_sample.position);
}










