
#include "BenchmarkScene.h"

#include "MainMenuScene.h"
#include "Benchmark/BenchmarkRunPresets.hpp"
#include "Components/FreeCameraComponent.h"

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

    // Create free cam
    m_cam     = CreateObject("BenchmarkCam");
    m_camComp = m_cam->AddComponent<RR::FreeCameraComponent>();
    m_cam->SetPosition(vec3(0.f, 120.f, 0.f));
    SetMainCamera(m_cam);

    SetCursorEnabled(false);
    m_camComp->SetDiscardInput(true);

    // PLACEHOLDER
    m_bench = RR::Engine::GetInstance().GetAppManager().GetSubSystem<RR::BenchmarkSubSystem>();

    // TEST PATH TO DEELTE!!!!!
    using BENCH::PathSegment;
    const vec3 spawn(0.f, 140.f, 0.f);
    m_path.Begin(spawn, 0.f, -15.f);

    PathSegment out;
    out.move = PathSegment::MOVE::GOTO;
    out.target = vec3(400.f, 140.f, -400.f);
    out.speed = 40.f;
    out.look = PathSegment::LOOK::FACE_TRAVEL;
    m_path.Add(out);

    PathSegment spin;
    spin.move = PathSegment::MOVE::HOLD;
    spin.holdSeconds = 6.f;
    spin.look = PathSegment::LOOK::ROTATE_YAW;
    spin.yawSweepDeg = 360.f;
    m_path.Add(spin);

    PathSegment back;                         
    back.move = PathSegment::MOVE::GOTO;
    back.target = spawn;
    back.speed = 40.f;
    back.look = PathSegment::LOOK::FACE_TRAVEL;
    m_path.Add(back);

    ApplyCameraSample(m_path.Sample(0.0f));
}

void BenchmarkScene::OnUpdate(float _deltaTime)
{
    if (m_paused) return;

    m_simTime += _deltaTime;
    ApplyCameraSample(m_path.Sample(m_simTime));

    // Path complete, proceed
    if (m_simTime >= m_path.Duration())
        LoadNextScene();
}

void BenchmarkScene::OnPauseEnter()
{
    VoxelScene::OnPauseEnter();
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










