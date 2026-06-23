
#include <cassert>

#include "VoxelScene.h"

#include "imgui.h"
#include "GLFW/glfw3.h"
#include "Components/FreeCameraComponent.h"
#include "Scenes/ArtefactMenu.h"
#include "Voxels/Chunk.h"
#include "Voxels/ChunkManager.h"
#include "WorldGen/WorldGen.hpp"

VoxelScene::VoxelScene(const std::string& _name, const RR::RunInfo& _runInfo,
    const WORLDGEN::WorldGenConfig& _genConfig)
    : Scene(_name), m_runInfo(_runInfo), m_genConfig(_genConfig)
{
    // RunInfo seed takes over
    m_genConfig.seed = _runInfo.seed;
}

// If not provided worlgen info use default
VoxelScene::VoxelScene(const std::string& _name, const RR::RunInfo& _runInfo)
    : Scene(_name), m_runInfo(_runInfo)
{
    m_genConfig.seed = _runInfo.seed;
}

VoxelScene::~VoxelScene()
= default;

bool VoxelScene::Init()
{
    // Save machine details
    m_runInfo.gpuName   = m_appData.gpuName;
    m_runInfo.cpuName   = m_appData.cpuName;
    m_runInfo.coreCount = m_appData.coreCount;

    // Load materials and assert entries are correct size
    m_voxelBlocksMat = RR::Material::Load("Materials/Voxel.json");
    {
        auto arr = m_voxelBlocksMat->GetTextureArray("uBlockTex");
        assert(arr && arr->GetLayerCount() == RR::CHUNK::BLOCKTEX::COUNT);
    }
    m_voxelVegMat = RR::Material::Load("Materials/VoxelVegetation.json");
    {
        auto vegArr = m_voxelVegMat->GetTextureArray("uBlockTex");
        assert(vegArr && vegArr->GetLayerCount() == RR::CHUNK::BLOCKTEX::COUNT);
    }

    // Chunk generator lambda
    RR::ChunkGenerator generator = [this](RR::Chunk& chunk) {
        WORLDGEN::GenerateColumn(chunk, m_genConfig);
    };

    // instantiate chunk manager with generator and mats
    m_chunkManager = std::make_unique<RR::ChunkManager>(
        generator, m_voxelBlocksMat, m_voxelVegMat);

    OnInit();
    return true;
}

void VoxelScene::PreUpdate(float _deltaTime)
{
    // Lock player in menu if ESC resume is not allowed
    if (m_paused && !m_allowResume) return;

    const auto& input = RR::Engine::GetInstance().GetInputManager();
    const bool escDown = input.IsKeyPressed(GLFW_KEY_ESCAPE);

    // Open pause menu if esc pressed
    if (escDown && !m_escWasDown)
    {
        m_paused = !m_paused;
        ApplyInputMode();

        // callbacks hook when menu open/closed
        if (m_paused)
        {
            OnPauseEnter();
        }
        else
        {
            OnPauseExit();
        }
    }
    m_escWasDown = escDown;
}

void VoxelScene::Update(float _deltaTime)
{
    if (m_chunkManager && m_cam)
    {
        const vec3 camPos = m_cam->GetWorldPosition();
        m_chunkManager->Update(camPos);
        m_chunkManager->SubmitDraws();
    }

    OnUpdate(_deltaTime);
}

void VoxelScene::LateUpdate(float _deltaTime)
{
}

void VoxelScene::Destroy()
{
}

void VoxelScene::OnGui()
{
    // Dont render anything if not paused
    if (!m_paused) return;

    const ImGuiViewport* viewPort = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewPort->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("PAUSED", nullptr, flags);

    //Size of buttons
    const ImVec2 buttonSize(220.0f, 0.0f);

    // menu entries
    if (ImGui::Button(m_pausePrimaryLabel.c_str(), buttonSize))
    {
        if (m_allowResume)
        {
            m_paused = false;
            ApplyInputMode();
            OnPauseExit();
            OnPausePrimary();

            ImGui::End();
            return;
        }

        OnPausePrimary();
    }
    if (ImGui::Button(m_pauseSecondaryLabel.c_str(), buttonSize))
    {
        OnPauseSecondary();
    }

    ImGui::End();
}

void VoxelScene::OnPauseSecondary()
{
    // Default go back to main menu
    RR::Engine::GetInstance().GetAppManager().RequestSceneLoad<ArtefactMenu>();
}

void VoxelScene::ApplyInputMode()
{
    if (!m_cam || !m_camComp) return;

    // Release cursor and discard camera input when in UI mode
    const bool uiActive = m_paused || InUiMode();
    SetCursorEnabled(uiActive);
    m_camComp->SetDiscardInput(uiActive);
}

bool VoxelScene::InUiMode() const
{
    // to be overloaded func if a scene has an external UI mode
    return false;
}

void VoxelScene::SetPrimaryButtonText(const std::string& _text)
{
    m_pausePrimaryLabel = _text;
}

void VoxelScene::SetSecondaryButtonText(const std::string& _text)
{
    m_pauseSecondaryLabel = _text;
}

void VoxelScene::SetResumable(bool _resumable)
{
    m_allowResume = _resumable;
}

// Optional hooks

void VoxelScene::OnPausePrimary()
{
}

void VoxelScene::OnPauseEnter()
{
}

void VoxelScene::OnPauseExit()
{
}

