#include <cstring>

#include "FreeRoam.h"

#include <random>

#include "Components/FreeCameraComponent.h"
#include "Render/MeshData.h"
#include "Render/Voxels/ChunkMesher.h"
#include "Voxels/Chunk.h"
#include "Voxels/ChunkData.h"
#include "Render/Mesh.h"
#include "Render/RenderQueue.h"
#include "Engine.h"
#include "GLFW/glfw3.h"
#include "Voxels/ChunkManager.h"
#include "imgui.h"
#include "WorldGen/Biome.hpp"
#include "WorldGen/BiomeMap.hpp"
#include "WorldGen/Noise.hpp"
#include "WorldGen/WorldGen.hpp"
#include "WorldGen/WorldGenConfig.h"

FreeRoam::FreeRoam() : Scene("Free Roam") {}

FreeRoam::~FreeRoam()
= default;

void FreeRoam::Regenerate()
{
    m_genConfig = m_draftConfig;
    m_chunkManager->Clear();
}

bool FreeRoam::Init()
{
    SetCursorEnabled(false);

    m_cam = CreateObject("FlyCam");
    m_camComp = m_cam->AddComponent<RR::FreeCameraComponent>();
    m_camComp->SetSprintSpeed(80.f);
    m_cam->SetPosition(vec3(0.f, 100.f, 0.f));
    SetMainCamera(m_cam);

    m_voxelMat = RR::Material::Load("Materials/Voxel.json");

    auto arr = m_voxelMat->GetTextureArray("uBlockTex");
    assert(arr && arr->GetLayerCount() == RR::CHUNK::BLOCKTEX::COUNT);


    RR::ChunkGenerator gen = [this](RR::Chunk& c) {
        WORLDGEN::GenerateColumn(c, m_genConfig);
    };
    m_chunkManager = std::make_unique<RR::ChunkManager>(gen, m_voxelMat);

    auto checkArea = [&](int ax, int az, int aw, int ah, const char* tag)
    {
        auto area = WORLDGEN::FinalArea(ax, az, aw, ah, m_genConfig);     // ← BuildArea, not ZoomArea
        bool ok = true;
        for (int j = 0; j < ah && ok; ++j)
            for (int i = 0; i < aw && ok; ++i)
                if (area[i + j * aw] != WORLDGEN::BiomeAtFinal(ax + i, az + j, m_genConfig)) ok = false;
        RR::InfoLog("[ZOOM] area==point ", tag, ": ", ok ? "PASS" : "FAIL");
    };
    checkArea(0, 0, 8, 8, "aligned");
    checkArea(3, -5, 7, 6, "offset-odd-neg");

    // proportions over many coarse cells (2048² fine ÷ 128 = 16² ≈ 256 coarse cells at L=7)
    {
        auto big = WORLDGEN::FinalArea(0, 0, 2048, 2048, m_genConfig);
        int zh[(int)WORLDGEN::BIOME::COUNT] = {};
        for (WORLDGEN::BIOME b : big) zh[(int)b]++;
        RR::InfoLog("[ZOOM] plains=", zh[0], " forest=", zh[1], " desert=", zh[2], " redDesert=", zh[3],
                    " taiga=", zh[4], " tundra=", zh[5], " mountains=", zh[6], " savanna=", zh[7]);
    }


    return true;
}

void FreeRoam::PreUpdate(float _deltaTime)
{
    auto& input = RR::Engine::GetInstance().GetInputManager();

    if (input.IsKeyPressed(GLFW_KEY_ESCAPE))
        RR::Engine::GetInstance().SetShouldClose(true);

    const bool tabDown = input.IsKeyPressed(GLFW_KEY_TAB);
    if (tabDown && !m_tabWasDown)
    {
        m_uiMode = !m_uiMode;
        SetCursorEnabled(m_uiMode);     // free cursor to click sliders
        m_camComp->SetDiscardInput(m_uiMode);
    }
    m_tabWasDown = tabDown;
}

void FreeRoam::Update(float _deltaTime)
{
    if (!m_chunkManager || !m_cam) return;
    const vec3 camPos = vec3(m_cam->GetWorldTransform()[3]);

    m_chunkManager->Update(camPos);
    m_chunkManager->SubmitDraws();
}

void FreeRoam::LateUpdate(float _deltaTime)
{
}

void FreeRoam::Destroy()
{
}

void FreeRoam::OnGui()
{
    // Passive FPS readout pinned to the top-right corner
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float pad = 10.0f;
    const ImVec2 pos(vp->WorkPos.x + vp->WorkSize.x - pad, vp->WorkPos.y + pad);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.5f);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration   | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav          | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoInputs;      

    if (ImGui::Begin("##fps_overlay", nullptr, flags))
    {
        const vec3 pos = m_cam->GetWorldPosition();

        const ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("FPS  %.1f", io.Framerate);
        ImGui::Text("%.3f ms/frame", 1000.0f / io.Framerate);
        ImGui::Text("X %.2f", pos.x);
        ImGui::Text("Y %.2f", pos.y);
        ImGui::Text("Z %.2f", pos.z);
    }
    ImGui::End();

    ImGui::Begin("World Gen", nullptr);

    static const char* kBiomeNames[(int)WORLDGEN::BIOME::COUNT] =
    { "Plains", "Forest", "Desert", "Red Desert", "Taiga", "Tundra", "Mountains", "Savanna" };

    if (ImGui::CollapsingHeader("Terrain", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Height scale", &m_draftConfig.heightScale, 16.f, 512.f);
        ImGui::SliderInt  ("Octaves",      &m_draftConfig.heightOctaves, 1, 8);
        ImGui::SliderInt  ("Dirt depth",   &m_draftConfig.dirtDepth, 1, 32);
    }

    if (ImGui::CollapsingHeader("Climate"))
    {
        ImGui::SliderFloat("Cold threshold",    &m_draftConfig.tempCold, 0.0f, 0.5f); // ≤ 0.5 so it can't pass Hot
        ImGui::SliderFloat("Hot threshold",     &m_draftConfig.tempHot, 0.5f, 1.0f);  // ≥ 0.5 so it can't pass Cold
        ImGui::SliderFloat("Mountain chance",   &m_draftConfig.mountainChance, 0.0f, 0.5f);
        ImGui::SliderFloat("Tundra humidity",   &m_draftConfig.TundraHumidThresh, 0.0f, 1.0f);
        ImGui::SliderFloat("Plains humidity",   &m_draftConfig.PlainsHumidThresh, 0.0f, 1.0f);
        ImGui::SliderFloat("Desert humidity",   &m_draftConfig.desertHumidThresh, 0.0f, 1.0f);
        ImGui::SliderFloat("Red desert rarity", &m_draftConfig.redDesertRarity, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Biome Map (cellular)"))
    {
        ImGui::SliderInt("Zoom levels",   &m_draftConfig.biomeZoomLevels, 1, 10);
        ImGui::SliderInt("Smooth passes", &m_draftConfig.biomeSmoothPasses, 0, 5);
        ImGui::SliderInt("Fuzzy levels",  &m_draftConfig.biomeFuzzyLevels, 0, 5);
    }

    if (ImGui::CollapsingHeader("Terrain Blend"))
    {
        ImGui::SliderInt  ("Blend radius",   &m_draftConfig.biomeBlendRadius, 1, 64);
        ImGui::SliderFloat("Mountain curve", &m_draftConfig.mountainCurve, 0.5f, 4.0f);
    }

    if (ImGui::CollapsingHeader("Biome Heights"))
    {
        for (int b = 0; b < (int)WORLDGEN::BIOME::COUNT; ++b)
        {
            ImGui::PushID(b);
            ImGui::Text("%s", kBiomeNames[b]);
            ImGui::SliderInt("base", &m_draftConfig.biomeBaseHeight[b], 0, 200);
            ImGui::SliderInt("amp",  &m_draftConfig.biomeAmplitude[b],  0, 120);
            ImGui::PopID();
        }
    }

    if (ImGui::CollapsingHeader("Mountain Surface"))
    {
        ImGui::SliderInt  ("Snow line",    &m_draftConfig.snowLine, 0, 256);
        ImGui::SliderInt  ("Grass line",   &m_draftConfig.mtnGrassLine, 0, m_draftConfig.snowLine); // capped at snow line
        ImGui::SliderFloat("Jitter scale", &m_draftConfig.snowJitterScale, 4.f, 200.f);
        ImGui::SliderFloat("Jitter amp",   &m_draftConfig.snowJitterAmp, 0.f, 40.f);
    }

    if (ImGui::CollapsingHeader("Strata & Ores"))
    {
        ImGui::SliderFloat("Strata scale",   &m_draftConfig.strataScale, 4.f, 100.f);
        ImGui::SliderFloat("Diorite thresh", &m_draftConfig.dioriteThresh, 0.0f, 1.0f);
        ImGui::SliderFloat("Granite thresh", &m_draftConfig.graniteThresh, 0.0f, 1.0f);
        ImGui::SliderFloat("Ore scale",      &m_draftConfig.oreScale, 2.f, 40.f);
        ImGui::SliderFloat("Coal thresh",    &m_draftConfig.coalThresh, 0.0f, 1.0f);
        ImGui::SliderFloat("Iron thresh",    &m_draftConfig.ironThresh, 0.0f, 1.0f);
        ImGui::SliderFloat("Copper thresh",  &m_draftConfig.copperThresh, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Water"))
    {
        ImGui::SliderInt  ("Water level",    &m_draftConfig.waterLevel, 0, 128);
        ImGui::SliderFloat("River scale",    &m_draftConfig.riverScale, 50.f, 600.f);
        ImGui::SliderFloat("River width",    &m_draftConfig.riverWidth, 0.0f, 0.3f);
        ImGui::SliderInt  ("River depth",    &m_draftConfig.riverDepth, 0, 32);
        ImGui::SliderInt  ("River octaves",  &m_draftConfig.riverNoiseOct, 1, 8);
        ImGui::SliderFloat("Pond scale",     &m_draftConfig.pondScale, 20.f, 300.f);
        ImGui::SliderFloat("Pond threshold", &m_draftConfig.pondThreshold, 0.0f, 0.5f);
        ImGui::SliderInt  ("Pond depth",     &m_draftConfig.pondDepth, 0, 32);
        ImGui::SliderInt  ("Pond octaves",   &m_draftConfig.pondNoiseOct, 1, 8);
    }

    ImGui::Separator();
    ImGui::InputScalar("Seed", ImGuiDataType_U32, &m_draftConfig.seed);
    ImGui::SameLine();
    if (ImGui::Button("Roll")) {
        std::mt19937 rng{std::random_device{}()};
        m_draftConfig.seed = rng();
    }
    if (ImGui::Button("Regenerate World")) Regenerate();

    ImGui::End();
}
