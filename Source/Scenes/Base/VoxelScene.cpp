
#include <cassert>
#include <cmath>
#include <algorithm>

#include "VoxelScene.h"

#include "imgui.h"
#include "GLFW/glfw3.h"
#include "Components/FreeCameraComponent.h"
#include "Scenes/MainMenuScene.h"
#include "Voxels/Chunk.h"
#include "../../../Engine/Source/Voxels/ChunkManager/ChunkManager.h"
#include "WorldGen/SurfaceLOD.hpp"
#include "Render/Voxels/SurfaceMesher.h"
#include "Voxels/LodNodeSelect.hpp"
#include "WorldGen/WorldGen.hpp"

namespace SHARED
{
    // Returns the base font size of the menu
    float PauseBaseFontSize()
    {
        const ImGuiStyle& style = ImGui::GetStyle();
        const float fontSize = ImGui::GetFontSize();
        const float scale    = style.FontScaleMain * style.FontScaleDpi;

        if (scale > 0.0f)
        {
            return fontSize / scale;
        }

        return fontSize;
    }
}

VoxelScene::VoxelScene(const RR::RunInfo& _runInfo, const WORLDGEN::WorldGenConfig& _genConfig)
    : Scene(_runInfo.scene), m_runInfo(_runInfo), m_genConfig(_genConfig)
{
    // RunInfo seed takes over
    m_genConfig.seed = _runInfo.seed;
}

// If not provided worlgen info use default
VoxelScene::VoxelScene(const RR::RunInfo& _runInfo)
    : Scene(_runInfo.scene), m_runInfo(_runInfo)
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

    // Threshold in between colors start dissolving and are fully dissolved
    constexpr float flatStart = 384.f; // 16 chunks
    constexpr float flatEnd   = 768.f; // 48 chunks
    m_voxelBlocksMat->SetParam("uFlatStart", flatStart);
    m_voxelBlocksMat->SetParam("uFlatEnd",   flatEnd);

    // Chunk generator lambda
    RR::ChunkGenerator generator = [config = m_genConfig](RR::Chunk& _chunk) {
        WORLDGEN::GenerateColumn(_chunk, config);
    };

    // LOD mesher lambda
    RR::LodMesher lodMesher = [config = m_genConfig, greedy = m_runInfo.greedy]
        (RR::LodNodeKey _key, int _coreEdges) -> RR::LodMeshResult
    {
        const WORLDGEN::SurfaceField field = WORLDGEN::ExtractSurface(
            _key.origin, _key.level, _key.footprint, _coreEdges, config);

        // proxies and surface mesh
        RR::LodMeshResult out;
        out.surface = RR::MeshSurface(field.dim, _key.level, config.lodBandMaxLevel,
            greedy, field.height, field.block, field.sideColumn);

        out.proxies = RR::MeshProxies(field.trees, _key.level);
        return out;
    };

    // instantiate chunk manager with generator and mats
    m_chunkManager = std::make_unique<RR::ChunkManager>(
        generator, lodMesher, m_voxelBlocksMat, m_voxelVegMat);

    // apply run info settings
    m_chunkManager->SetRenderDistance(m_runInfo.renderDistance);
    m_chunkManager->SetLodEnabled(m_runInfo.lod);
    m_chunkManager->SetAggregationEnabled(m_runInfo.aggregation);
    m_chunkManager->SetAsyncEnabled(m_runInfo.async);
    m_chunkManager->SetAdaptiveBudgetingEnabled(m_runInfo.scheduling);

    // Set sky box color
    constexpr vec3 skyBoxColor(0.58f, 0.73f, 0.93f);
    SetSceneClearColor(vec4(skyBoxColor, 1.0f));

    // Fog starts at core and expands with generated land
    m_fogEnd = RR::ChunkManager::kDefaultCoreRadius * RR::CHUNK::kSizeX;

    for (const auto& mat : { m_voxelBlocksMat, m_voxelVegMat })
    {
        mat->SetParam("uFogColor", skyBoxColor);
        mat->SetParam("uFogStart", m_fogEnd * 0.95f);
        mat->SetParam("uFogEnd",   m_fogEnd);
    }

    // Skybox
    m_skybox = std::make_unique<RR::VoxelSkybox>();
    m_skybox->Load("Materials/Skybox.json", "Materials/Clouds.json");

    m_skybox->SetHorizonColor(skyBoxColor);
    m_skybox->SetZenithColor (vec3(0.30f, 0.52f, 0.88f));
    m_skybox->SetSun(vec3(0.35f, 0.65f, 0.45f), 0.22f);

    // clouds
    m_skybox->SetCloudSeed(m_genConfig.seed);
    m_skybox->SetCloudHeight(230);
    m_skybox->SetCloudFade(0.85f, 0.4f);
    m_skybox->SetCloudColor(vec3(1.0f));
    m_skybox->SetWind(vec2(1.0f, 0.35f), 2.0f);
    m_skybox->SetCloudsEnabled(false);

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
    if (m_chunkManager && m_cam && m_camComp)
    {
        const vec3 camPos = m_cam->GetWorldPosition();
        m_chunkManager->Update(_deltaTime, camPos);

        // area fraction to expand fog
        const float fullEnd = static_cast<float>(m_runInfo.renderDistance) * RR::CHUNK::kSizeX * 0.95f;
        const float minEnd  = RR::ChunkManager::kDefaultCoreRadius * RR::CHUNK::kSizeX;
        const float target  = std::max(minEnd, fullEnd * std::sqrt(m_chunkManager->GetCoverage()));

        m_fogEnd += (target - m_fogEnd) * std::min(1.0f, _deltaTime * m_fogMoveSpeed);

        // apply fog smoothing
        for (const auto& mat : { m_voxelBlocksMat, m_voxelVegMat })
        {
            mat->SetParam("uFogStart", m_fogEnd * 0.90f);
            mat->SetParam("uFogEnd",   m_fogEnd);
        }

        // RenderSky
        m_skybox->UpdateClouds(camPos, _deltaTime);
        m_skybox->SubmitSkyForDraw();

        // calculate camera frustum
        const float aspect   = RR::Engine::GetInstance().GetAspectRatio();
        const mat4  viewProj = m_camComp->GetProjectionMatrix(aspect) *
                               m_camComp->GetViewMatrix();
        const bool  revZ     = RR::Engine::GetInstance().GetGraphicsAPI().IsReversedZ();
        const RR::Frustum ft = RR::Frustum::FromViewProj(viewProj, revZ);

        // submit chunks for rendering
        m_chunkManager->SubmitDraws(ft);

        // Render clodus
        m_skybox->SubmitCloudsForDraw();
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
    Scene::OnGui();

    if (!m_paused) return;

    const ImGuiViewport* viewPort = ImGui::GetMainViewport();

    // Draw a panel to dim the screen
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        viewPort->Pos,
        ImVec2(viewPort->Pos.x + viewPort->Size.x, viewPort->Pos.y + viewPort->Size.y),
        IM_COL32(0, 0, 0, m_dimStrenght));

    // Main menu over the centre, text + buttons
    ImGui::SetNextWindowPos(viewPort->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(viewPort->WorkSize.x * 0.5f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.95f);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration    | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("##pause_menu", nullptr, flags);

    const float  baseFont = SHARED::PauseBaseFontSize();
    const float  startX   = ImGui::GetCursorPosX();
    const float  contentW = ImGui::GetContentRegionAvail().x;
    const ImVec2 buttonSize(contentW * buttonWidthFrac, baseFont * buttonHeight);
    const float  buttonX  = startX + (contentW - buttonSize.x) * 0.5f;

    // Title — centred over the panel
    ImGui::PushFont(ImGui::GetFont(), baseFont * titleFontScale);
    {
        const float titleW = ImGui::CalcTextSize("PAUSED").x;

        ImGui::Dummy(ImVec2(0.0f, baseFont * 0.5f));
        ImGui::SetCursorPosX(startX + (contentW - titleW) * 0.5f);
        ImGui::TextUnformatted("PAUSED");
    }
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, baseFont * buttonsTitleSpacing));

    // Buttons
    ImGui::PushFont(ImGui::GetFont(), baseFont * buttonsTextSize);

    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button(m_pausePrimaryLabel.c_str(), buttonSize))
    {
        // If primary option is chosen
        if (m_allowResume)
        {
            m_paused = false;
            ApplyInputMode();
            OnPauseExit();
            OnPausePrimary();

            ImGui::PopFont();
            ImGui::End();
            return;
        }
        OnPausePrimary();
    }

    ImGui::Dummy(ImVec2(0.0f, baseFont * buttonsInnerSpacing));

    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button(m_pauseSecondaryLabel.c_str(), buttonSize))
    {
        //if secondary option is chosen
        OnPauseSecondary();
    }

    ImGui::Dummy(ImVec2(0.0f, baseFont * buttonsTitleSpacing));
    ImGui::Dummy(ImVec2(0.0f, baseFont * buttonsTitleSpacing));

    ImGui::PopFont();
    ImGui::End();
}

void VoxelScene::OnPauseSecondary()
{
    // Default go back to main menu
    RR::Engine::GetInstance().GetAppManager().RequestSceneLoad<MainMenuScene>();
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

