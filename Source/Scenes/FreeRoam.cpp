#include <cstring>

#include "FreeRoam.h"

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
#include "WorldGen/Noise.hpp"

FreeRoam::FreeRoam() : Scene("Free Roam") {}

FreeRoam::~FreeRoam()
= default;

static void FillTestSlab(RR::Chunk& _c)
{
    using namespace RR::CHUNK;
    const BlockId grass = static_cast<BlockId>(BLOCK::GRASS);
    const BlockId dirt  = static_cast<BlockId>(BLOCK::DIRT);
    const BlockId stone = static_cast<BlockId>(BLOCK::STONE);

    for (int z = 0; z < kSizeZ; ++z)
        for (int x = 0; x < kSizeX; ++x)
        {
            _c.Set(x, 0, z, stone);
            _c.Set(x, 1, z, stone);
            _c.Set(x, 2, z, dirt);
            _c.Set(x, 3, z, dirt);
            _c.Set(x, 4, z, grass);
        }
}

bool FreeRoam::Init()
{
    SetCursorEnabled(false);

    m_cam = CreateObject("FlyCam");
    auto camComp = m_cam->AddComponent<RR::FreeCameraComponent>();
    camComp->SetSprintSpeed(26.f);
    SetMainCamera(m_cam);

    m_voxelMat = RR::Material::Load("Materials/Voxel.json");

    auto arr = m_voxelMat->GetTextureArray("uBlockTex");
    assert(arr && arr->GetLayerCount() == CHUNK::BLOCKTEX::COUNT);

    RR::ChunkGenerator gen = [](RR::Chunk& c) {
        FillTestSlab(c);
    };
    m_chunkManager = std::make_unique<RR::ChunkManager>(gen, m_voxelMat);

    float mn = 1e9f, mx = -1e9f, maxStep = 0.0f, prev = WorldGen::FBM(0,0,1337,4);
    for (int i = 1; i < 2000; ++i)
    {
        float v = WorldGen::FBM(i * 0.01f, 0.0f, 1337, 4);
        mn = std::min(mn, v);  mx = std::max(mx, v);
        maxStep = std::max(maxStep, std::abs(v - prev));  prev = v;
    }
    RR::InfoLog("[NOISE] min=", mn, " max=", mx, " maxStep=", maxStep);

    return true;
}

void FreeRoam::PreUpdate(float _deltaTime)
{
    auto& input = RR::Engine::GetInstance().GetInputManager();

    if (input.IsKeyPressed(GLFW_KEY_ESCAPE))
        RR::Engine::GetInstance().SetShouldClose(true);
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
        const ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("FPS  %.1f", io.Framerate);
        ImGui::Text("%.3f ms/frame", 1000.0f / io.Framerate);
    }
    ImGui::End();
}
