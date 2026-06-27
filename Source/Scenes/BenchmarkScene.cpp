
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
            m_warmUpStart        = std::chrono::steady_clock::now();
            m_lastProgressTime   = m_warmUpStart;
            m_warmUpTimerStarted = true;
        }

        // load timeout - abort if loading takes too long
        const float warmElapsed = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - m_warmUpStart).count();

        if (warmElapsed > kMaxWarmUpSeconds)
        {
            RR::Warn("[BENCHMARK] Loading terrain exceeded timeout, this machine can't load this config, aborting");
            AbortRun();
            return;
        }

        // stuck watchdog - abort if terrain stops generating
        const float coverage = m_chunkManager->GetCoverage();
        if (coverage > m_lastCoverage)
        {
            m_lastCoverage     = coverage;
            m_lastProgressTime = std::chrono::steady_clock::now();
        }
        else if (std::chrono::duration<float>(
            std::chrono::steady_clock::now() - m_lastProgressTime).count() > kStuckSeconds)
        {
            RR::Warn("[BENCHMARK] Loading stalled — no streaming progress, aborting");
            AbortRun();
            return;
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

    // stack up "slow" frames if the application is lagging too much
    if (_deltaTime > kAbortFrameSeconds)
    {
        ++m_slowFrames;
    }
    else
    {
        m_slowFrames = 0;
    }

    // check if the camera is not standing on an ungenerate chunk
    const bool nullTerrain = !m_chunkManager->IsChunkMeshedAt(m_cam->GetWorldPosition());

    if (m_slowFrames >= kAbortSlowFrames || nullTerrain)
    {
        RR::Warn("[BENCHMARK] Aborting run, sustained consistent lag or went over null terrain");
        AbortRun();
        return;
    }

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
        gDeterministicFailures = 0;

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
    appMan.RequestSceneLoad<MainMenuScene>(BENCH_SUCCESS::CANCELLED);
}

void BenchmarkScene::AbortRun()
{
    if (m_pathComplete) return;

    m_pathComplete = true;

    // discard fail
    if (m_bench) m_bench->RequestDiscard();

    // deterministic, remeber if any fails
    if (m_runInfo.deterministic)
    {
        ++DETERMINISTIC::gDeterministicFailures;
        LoadNextScene();
    }
    else
    {
        RR::Engine::GetInstance().GetAppManager().RequestSceneLoad<MainMenuScene>(BENCH_SUCCESS::ABORTED);
    }
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
        const BENCH_SUCCESS outcome = (gDeterministicFailures == 0)
            ? BENCH_SUCCESS::DETERMINISTIC
            : BENCH_SUCCESS::DETERMINISTIC_PARTIAL;
        RR::Success("[DETERMINISTIC BENCHMARK] Deterministic benchmark has concluded");
        appMan.RequestSceneLoad<MainMenuScene>(outcome);
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










