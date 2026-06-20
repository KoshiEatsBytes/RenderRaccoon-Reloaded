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

    m_vegMat = RR::Material::Load("Materials/VoxelVegetation.json");
    auto vegArr = m_vegMat->GetTextureArray("uBlockTex");
    assert(vegArr && vegArr->GetLayerCount() == RR::CHUNK::BLOCKTEX::COUNT);


    RR::ChunkGenerator gen = [this](RR::Chunk& c) {
        WORLDGEN::GenerateColumn(c, m_genConfig);
    };
    m_chunkManager = std::make_unique<RR::ChunkManager>(gen, m_voxelMat, m_vegMat);

    auto checkArea = [&](int ax, int az, int aw, int ah, const char* tag)
    {
        auto area = WORLDGEN::FinalArea(ax, az, aw, ah, m_genConfig);    
        bool ok = true;
        for (int j = 0; j < ah && ok; ++j)
            for (int i = 0; i < aw && ok; ++i)
                if (area[i + j * aw] != WORLDGEN::BiomeAtFinal(ax + i, az + j, m_genConfig)) ok = false;
        RR::InfoLog("[ZOOM] area==point ", tag, ": ", ok ? "PASS" : "FAIL");
    };
    checkArea(0, 0, 8, 8, "aligned");
    checkArea(3, -5, 7, 6, "offset-odd-neg");

    // proportions over many coarse cells
    {
        auto big = WORLDGEN::FinalArea(0, 0, 2048, 2048, m_genConfig);
        int zh[(int)WORLDGEN::BIOME::COUNT] = {};
        for (WORLDGEN::BIOME b : big) zh[(int)b]++;
        RR::InfoLog("[ZOOM] plains=", zh[0], " forest=", zh[1], " desert=", zh[2], " mesa=", zh[3],
                    " taiga=", zh[4], " tundra=", zh[5], " mountains=", zh[6], " savanna=", zh[7]);
    }

    {
        using namespace RR::CHUNK;

        bool reproducible = true;
        for (Coord c : { Coord{0, 0}, Coord{-3, 5}, Coord{12, -7} })
        {
            RR::Chunk a(c), b(c);
            WORLDGEN::GenerateColumn(a, m_genConfig);
            WORLDGEN::GenerateColumn(b, m_genConfig);
            if (a.voxels != b.voxels) reproducible = false;     
        }
        RR::InfoLog("[DETERMINISM] same seed twice == identical: ", reproducible ? "PASS" : "FAIL");

        WORLDGEN::WorldGenConfig other = m_genConfig;
        other.seed += 1u;
        RR::Chunk s0(Coord{0, 0}), s1(Coord{0, 0});
        WORLDGEN::GenerateColumn(s0, m_genConfig);
        WORLDGEN::GenerateColumn(s1, other);
        RR::InfoLog("[DETERMINISM] seed changes the world: ", (s0.voxels != s1.voxels) ? "PASS" : "FAIL");
    }



    {
        const int cx = 3, cz = -2;
        const int margin = m_genConfig.biomeBlendRadius;
        const auto grid = WORLDGEN::BuildBiomeGrid(cx, cz, margin, m_genConfig);
        const auto sums = WORLDGEN::BuildBlendSums(grid, m_genConfig);
        const int total = (2*margin+1) * (2*margin+1);
        bool ok = true;
        for (int z = 0; z < 16 && ok; ++z)
            for (int x = 0; x < 16; ++x)
            {
                const int wx = cx*16 + x, wz = cz*16 + z;
                const int chunkLand = WORLDGEN::TerrainHeightFromSums(sums[x + z*16], wx, wz, total, m_genConfig);
                if (chunkLand != WORLDGEN::LandHeight(wx, wz, m_genConfig)) { ok = false; break; }
            }
        RR::InfoLog("[LANDHEIGHT] oracle ", ok ? "PASS" : "FAIL");
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
        SetCursorEnabled(m_uiMode);     
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
    // Passive FPS readout
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

    // World Gen panel 
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Once);
    ImGui::SetNextWindowCollapsed(!m_uiMode, ImGuiCond_Always);
    ImGui::Begin("World Gen (PRESS TAB)", nullptr, ImGuiWindowFlags_NoSavedSettings);

    static const char* kBiomeNames[(int)WORLDGEN::BIOME::COUNT] =
    { 
        "Plains", 
        "Forest", 
        "Desert", 
        "Mesa", 
        "Taiga", 
        "Tundra", 
        "Mountains", 
        "Savanna" 
        };

    if (ImGui::CollapsingHeader("Terrain Shape", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SeparatorText("Base noise");
        ImGui::Checkbox   ("Gradient noise (Perlin)", &m_draftConfig.useGradientNoise);
        ImGui::SliderFloat("Height scale", &m_draftConfig.heightScale, 16.f, 512.f);
        ImGui::SliderInt  ("Octaves",      &m_draftConfig.heightOctaves, 1, 8);
        ImGui::SliderInt  ("Dirt depth",   &m_draftConfig.dirtDepth, 1, 32);

        ImGui::SeparatorText("Domain warp");
        ImGui::Checkbox   ("Enabled##warp", &m_draftConfig.warpEnabled);
        ImGui::SliderFloat("Scale##warp",   &m_draftConfig.warpScale, 20.f, 1024.f);
        ImGui::SliderFloat("Amp##warp",     &m_draftConfig.warpAmp, 1.f, 384.f);
        ImGui::SliderInt  ("Octaves##warp", &m_draftConfig.warpOctaves, 1, 10);
        ImGui::SliderInt  ("Levels##warp", &m_draftConfig.warpLevels, 1, 2);

        ImGui::SeparatorText("Detail / terracing");
        ImGui::Checkbox   ("Enabled##detail", &m_draftConfig.detailEnabled);
        ImGui::SliderFloat("Scale##detail",   &m_draftConfig.detailScale, 1.f, 256.f);
        ImGui::SliderFloat("Amp##detail",     &m_draftConfig.detailAmp, 0.1f, 10.f);
        ImGui::SliderInt  ("Octaves##detail", &m_draftConfig.detailOctaves, 1, 10);

        ImGui::SeparatorText("Mountain ridges");
        ImGui::Checkbox   ("Enabled##ridge",  &m_draftConfig.ridgeMountains);
        ImGui::SliderFloat("Strength##ridge", &m_draftConfig.ridgeStrength, 0.01f, 1.f);
    }

    if (ImGui::CollapsingHeader("Climate (biome selection)"))
    {
        ImGui::SliderFloat("Cold threshold",    &m_draftConfig.tempCold, 0.0f, 0.5f);
        ImGui::SliderFloat("Hot threshold",     &m_draftConfig.tempHot, 0.5f, 1.0f); 
        ImGui::SliderFloat("Mountain chance",   &m_draftConfig.mountainChance, 0.0f, 0.5f);
        ImGui::SliderFloat("Tundra humidity",   &m_draftConfig.tundraHumidThresh, 0.0f, 1.0f);
        ImGui::SliderFloat("Plains humidity",   &m_draftConfig.plainsHumidThresh, 0.0f, 1.0f);
        ImGui::SliderFloat("Desert humidity",   &m_draftConfig.desertHumidThresh, 0.0f, 1.0f);
        ImGui::SliderFloat("Mesa rarity", &m_draftConfig.mesaRarity, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Biome Map (cellular)"))
    {
        ImGui::SliderInt("Zoom levels",   &m_draftConfig.biomeZoomLevels, 1, 10);
        ImGui::SliderInt("Smooth passes", &m_draftConfig.biomeSmoothPasses, 0, 5);
        ImGui::SliderInt("Fuzzy levels",  &m_draftConfig.biomeFuzzyLevels, 0, 5);
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

    if (ImGui::CollapsingHeader("Terrain Blend"))
    {
        ImGui::SliderInt  ("Blend radius",   &m_draftConfig.biomeBlendRadius, 1, 64);
        ImGui::SliderFloat("Mountain curve", &m_draftConfig.mountainCurve, 0.5f, 4.0f);
    }

    if (ImGui::CollapsingHeader("Mountain Surface"))
    {
        ImGui::SliderInt  ("Snow line",    &m_draftConfig.snowLine, 0, 256);
        ImGui::SliderInt  ("Grass line",   &m_draftConfig.mtnGrassLine, 0, m_draftConfig.snowLine); // capped at snow line
        ImGui::SliderFloat("Jitter scale", &m_draftConfig.snowJitterScale, 4.f, 200.f);
        ImGui::SliderFloat("Jitter amp",   &m_draftConfig.snowJitterAmp, 0.f, 40.f);
    }

    if (ImGui::CollapsingHeader("Mesa / Cliffs"))
    {
        ImGui::SeparatorText("Shape");
        ImGui::SliderFloat("Noise scale##mesa",   &m_draftConfig.mesaNoiseScale, 50.f, 600.f);
        ImGui::SliderInt  ("Noise octaves##mesa", &m_draftConfig.mesaNoiseOctaves, 1, 8);
        ImGui::TextDisabled("(mesa base/amp live in Biome Heights)");

        ImGui::SeparatorText("Cliffs");
        ImGui::Checkbox   ("Enabled##cliff",      &m_draftConfig.cliffsEnabled);
        ImGui::SliderFloat("Cluster scale##cliff",&m_draftConfig.cliffScale, 1.f, 600.f);
        ImGui::SliderInt  ("Octaves##cliff",      &m_draftConfig.cliffOctaves, 1, 6);
        ImGui::SliderFloat("Threshold##cliff",    &m_draftConfig.cliffThreshold, 0.0f, 1.0f);
        ImGui::SliderFloat("Blend width##cliff",  &m_draftConfig.cliffBlendWidth, 0.01f, 0.5f);
        ImGui::SliderFloat("Step (face)##cliff",  &m_draftConfig.cliffStep, 2.0f, 32.0f);
        ImGui::SliderFloat("Riser##cliff",        &m_draftConfig.cliffRiser, 0.05f, 1.0f);
        ImGui::SliderFloat("Phase scale##cliff",  &m_draftConfig.cliffPhaseScale, 50.f, 600.f);

        ImGui::SeparatorText("Bands");
        ImGui::SliderFloat("Thickness##band",     &m_draftConfig.mesaBandThickness, 1.0f, 16.0f);
        ImGui::SliderFloat("Jitter scale##band",  &m_draftConfig.mesaBandJitterScale, 4.f, 200.f);
        ImGui::SliderFloat("Jitter amp##band",    &m_draftConfig.mesaBandJitterAmp, 0.f, 16.f);

        ImGui::SeparatorText("Apron");
        ImGui::Checkbox   ("Mesa apron",          &m_draftConfig.mesaApron);
        ImGui::SliderFloat("Mesa thresh##apron",  &m_draftConfig.mesaApronThresh, 0.0f, 0.99f);
        ImGui::Checkbox   ("Mountain apron",      &m_draftConfig.mtnApron);
        ImGui::SliderFloat("Mtn thresh##apron",   &m_draftConfig.mtnApronThresh, 0.0f, 0.99f);

        ImGui::SeparatorText("Rim & Buffer");
        ImGui::Checkbox   ("Rim cliffs",              &m_draftConfig.mesaRimCliffs);
        ImGui::Checkbox   ("Mesa-Mtn plains buffer",  &m_draftConfig.mesaMtnBuffer);
        ImGui::SliderInt  ("Buffer radius",           &m_draftConfig.mesaMtnBufferRadius, 0, 48);
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
        ImGui::SliderInt  ("Water level",        &m_draftConfig.waterLevel, 0, 128);
        ImGui::Checkbox   ("Ice in cold biomes", &m_draftConfig.iceEnabled);

        ImGui::SeparatorText("Rivers");
        ImGui::Checkbox   ("Rivers through taiga", &m_draftConfig.taigaRivers);
        ImGui::SliderInt  ("River level",        &m_draftConfig.riverLevel, 0, 128);
        ImGui::SliderInt  ("Channel depth",      &m_draftConfig.riverDepth, 0, 32);
        ImGui::SliderInt  ("Shelf depth",        &m_draftConfig.riverShelfDepth, 0, 32);
        ImGui::SliderFloat("Channel threshold",  &m_draftConfig.channelThreshold, 0.0f, 1.0f);
        ImGui::SliderFloat("River scale",        &m_draftConfig.riverScale, 50.f, 600.f);
        ImGui::SliderInt  ("River octaves",      &m_draftConfig.riverNoiseOct, 1, 8);
        ImGui::SliderFloat("Half width (blocks)",&m_draftConfig.riverHalfWidth, 1.f, 24.f);
        ImGui::SliderFloat("Grad cull",          &m_draftConfig.riverGradMin, 0.0f, 0.40f, "%.3f");
        ImGui::SliderFloat("Bank sharp mtn",     &m_draftConfig.riverBankSharpnessMtn, 1.0f, 8.0f);
        ImGui::SliderFloat("Bank sharp mesa",    &m_draftConfig.riverBankSharpnessMesa, 1.0f, 8.0f);
        ImGui::SliderInt  ("Max height (fade)",  &m_draftConfig.riverMaxHeight, 0, 200);
        ImGui::SliderFloat("Fade range",         &m_draftConfig.riverFade, 1.f, 64.f);

        ImGui::SeparatorText("River warp");
        ImGui::Checkbox   ("Enabled##rwarp",     &m_draftConfig.riverWarpEnabled);
        ImGui::SliderFloat("Scale##rwarp",       &m_draftConfig.riverWarpScale, 20.f, 600.f);
        ImGui::SliderFloat("Amp##rwarp",         &m_draftConfig.riverWarpAmp, 0.f, 200.f);

        ImGui::SeparatorText("Beaches");
        ImGui::SliderFloat("Beach band",            &m_draftConfig.beachBand, 0.0f, 0.5f);
        ImGui::SliderFloat("Sand density",          &m_draftConfig.beachSandChance, 0.0f, 1.0f);
        ImGui::Checkbox   ("Desert -> grass banks", &m_draftConfig.desertRiverGrass);

        ImGui::SeparatorText("Tunnels (karst)");
        ImGui::Checkbox   ("Enabled##tunnel",   &m_draftConfig.riverTunnels);
        ImGui::SliderFloat("Biome gate mtn",    &m_draftConfig.tunnelMaskThreshMtn, 0.0f, 1.0f);
        ImGui::SliderFloat("Biome gate mesa",   &m_draftConfig.tunnelMaskThreshMesa, 0.0f, 1.0f);
        ImGui::SliderInt  ("Bore rise mtn",     &m_draftConfig.tunnelRiseMtn, 0, 64);
        ImGui::SliderInt  ("Bore rise mesa",    &m_draftConfig.tunnelRiseMesa, 0, 64);
        ImGui::SliderFloat("Arch height",       &m_draftConfig.riverArchHeight, 2.0f, 30.0f);
        ImGui::SliderFloat("Ceiling scale",     &m_draftConfig.riverCeilScale, 6.0f, 64.0f);
        ImGui::SliderFloat("Ceiling jitter",    &m_draftConfig.riverCeilJitter, 0.0f, 16.0f);
        ImGui::SliderFloat("Formation density", &m_draftConfig.calciteChance, 0.0f, 1.0f);
        ImGui::SliderInt  ("Formation band",    &m_draftConfig.calciteBand, 0, 8);
        ImGui::SliderFloat("Dripstone frac",    &m_draftConfig.dripstoneFraction, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Vegetation"))
    {
        ImGui::SeparatorText("Trees");
        ImGui::SliderInt("Max slope", &m_draftConfig.treeSlopeMax, 0, 16);

        ImGui::SeparatorText("Clumping (thickets & clearings)");
        ImGui::Checkbox   ("Enabled##clump",    &m_draftConfig.clumpEnabled);
        ImGui::SliderFloat("Scale##clump",      &m_draftConfig.clumpScale, 20.f, 400.f);
        ImGui::SliderFloat("Warp##clump",       &m_draftConfig.clumpWarp, 0.f, 120.f);
        ImGui::SliderFloat("Clear line##clump", &m_draftConfig.clumpClear, 0.0f, m_draftConfig.clumpThick); 
        ImGui::SliderFloat("Thick line##clump", &m_draftConfig.clumpThick, m_draftConfig.clumpClear, 1.0f); 
        ImGui::SliderFloat("Peak##clump",       &m_draftConfig.clumpPeak, 0.5f, 5.0f);

        ImGui::SeparatorText("Per-biome");
        for (int b = 0; b < (int)WORLDGEN::BIOME::COUNT; ++b)
        {
            WORLDGEN::BiomeVeg& v = m_draftConfig.biomeVegetation[b];
            ImGui::PushID(b);
            if (ImGui::TreeNode(kBiomeNames[b]))
            {
                ImGui::SeparatorText("Trees");
                ImGui::SliderFloat("Density##tree", &v.tree,          0.0f, 0.10f, "%.4f");
                ImGui::SliderFloat("Clump amount",  &v.clumpAmount,   0.0f, 1.0f);
                ImGui::SliderInt  ("Min height",    &v.treeMinHeight, 0, 40);
                ImGui::SliderInt  ("Max height",    &v.treeMaxHeight, v.treeMinHeight, 48); 

                ImGui::SeparatorText("Ground cover");
                ImGui::SliderFloat("Grass",      &v.grass,     0.0f, 1.0f);
                ImGui::SliderFloat("Tall grass", &v.tallGrass, 0.0f, 1.0f);
                ImGui::SliderFloat("Flower",     &v.flower,    0.0f, 0.10f, "%.4f");
                ImGui::SliderFloat("Bush",       &v.bush,      0.0f, 0.10f, "%.4f");
                ImGui::SliderFloat("Cactus",     &v.cactus,    0.0f, 0.05f, "%.4f");
                ImGui::SliderFloat("Boulder",    &v.boulder,   0.0f, 0.05f, "%.4f");
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }

    if (ImGui::CollapsingHeader("Rendering"))
    {
        if (ImGui::Checkbox("Fancy leaves", &m_fancyLeaves))
        {
            m_chunkManager->SetFancyLeaves(m_fancyLeaves);
        }
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
