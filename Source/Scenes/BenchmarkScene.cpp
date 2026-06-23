
#include "BenchmarkScene.h"
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
}

void BenchmarkScene::OnUpdate(float _deltaTime)
{
}

void BenchmarkScene::OnPauseEnter()
{
    VoxelScene::OnPauseEnter();
}

void BenchmarkScene::OnPausePrimary()
{
    // Restart scene this exact scene
    RR::Engine::GetInstance().GetAppManager().RequestSceneLoad<BenchmarkScene>(
        m_runInfo, m_genConfig);
}