#include "MultiPitBenchmark.h"
#include "Game.h"
#include "GLFW/glfw3.h"
#include <cmath>

MultiPitBenchmark::MultiPitBenchmark() : Scene("MultiPitBenchmark") {}

bool MultiPitBenchmark::Init()
{
    m_material = RR::Material::Load("Materials/Brick.json");
    m_boxMesh  = RR::Mesh::CreateBox(vec3(1.0f));

    const int   numPits     = kPitsPerSide * kPitsPerSide;
    const int   boxesPerPit = kTotalBoxes / numPits;

    // Size each pit so fill depth is constant
    m_pitHalf = 12.0f * std::sqrt(static_cast<float>(boxesPerPit) / 1000.0f);
    const float cellPitch  = m_pitHalf * 2.0f + 4.0f;   // walls + a gap so islands never merge
    const float gridExtent = kPitsPerSide * cellPitch;

    auto camera = CreateObject("Camera");
    camera->AddComponent(new RR::CameraComponent());
    camera->SetPosition(vec3(0.0f, gridExtent * 1.2f, gridExtent * 0.85f));
    camera->SetRotation(quat(vec3(-0.95f, 0.0f, 0.0f)));   // ~-54° look-down
    SetMainCamera(camera);

    auto light = CreateObject("Light");
    auto lc = new RR::LightComponent();
    lc->SetColor(vec3(1.0f));
    light->AddComponent(lc);
    light->SetPosition(vec3(0.0f, gridExtent + 20.0f, gridExtent * 0.4f));

    // Ground covering the whole grid
    {
        const float g = gridExtent + 8.0f;
        auto ground = CreateObject("Ground");
        ground->SetPosition(vec3(0.0f, -5.0f, 0.0f));
        ground->AddComponent(new RR::MeshComponent(m_material, RR::Mesh::CreateBox(vec3(g, 2.0f, g))));
        ground->AddComponent(new RR::ColliderComponent(std::make_shared<RR::BoxCollider>(vec3(g, 2.0f, g))));
        ground->AddComponent(new RR::PhysicsComponent(RR::BodyType::STATIC, 0.0f, 0.8f));
    }

    // Grid of independent pits centered on origin
    const float offset = (kPitsPerSide - 1) * 0.5f;
    for (int i = 0; i < kPitsPerSide; ++i)
        for (int j = 0; j < kPitsPerSide; ++j)
        {
            const vec3 center((i - offset) * cellPitch, 0.0f, (j - offset) * cellPitch);
            m_pitCenters.push_back(center);
            BuildPit(center, m_pitHalf);
        }

    RR::Success("[MULTIPIT] ", numPits, " pits | ", kTotalBoxes, " boxes (",
                boxesPerPit, "/pit) | pitHalf=", m_pitHalf);
    return true;
}

void MultiPitBenchmark::BuildPit(const vec3& _center, float _half)
{
    const float wt   = 1.0f;
    const float span = _half * 2.0f + wt * 2.0f;

    auto wall = [&](const vec3& _pos, const vec3& _ext)
    {
        auto w = CreateObject("Wall");
        w->SetPosition(_pos);
        w->AddComponent(new RR::MeshComponent(m_material, RR::Mesh::CreateBox(_ext)));
        w->AddComponent(new RR::ColliderComponent(std::make_shared<RR::BoxCollider>(_ext)));
        w->AddComponent(new RR::PhysicsComponent(RR::BodyType::STATIC, 0.0f, 0.5f));
    };

    wall(_center + vec3(0.0f, 0.0f, -(_half + wt * 0.5f)), vec3(span, kWallHeight, wt));
    wall(_center + vec3(0.0f, 0.0f,  (_half + wt * 0.5f)), vec3(span, kWallHeight, wt));
    wall(_center + vec3( (_half + wt * 0.5f), 0.0f, 0.0f), vec3(wt, kWallHeight, span));
    wall(_center + vec3(-(_half + wt * 0.5f), 0.0f, 0.0f), vec3(wt, kWallHeight, span));
}

void MultiPitBenchmark::SpawnBox(const vec3& _pos, const quat& _rot)
{
    auto box = CreateObject("Box");
    box->SetPosition(_pos);
    box->SetRotation(_rot);
    box->AddComponent(new RR::MeshComponent(m_material, m_boxMesh));
    box->AddComponent(new RR::ColliderComponent(std::make_shared<RR::BoxCollider>(vec3(1.0f))));
    box->AddComponent(new RR::PhysicsComponent(RR::BodyType::DYNAMIC, 1.0f, 0.5f));
    ++m_boxCount;
}

void MultiPitBenchmark::PreUpdate(float) {}

void MultiPitBenchmark::Update(float _deltaTime)
{
    auto& input = RR::Engine::GetInstance().GetInputManager();
    if (input.IsKeyPressed(GLFW_KEY_1))
        RR::Engine::GetInstance().GetAppManager().RequestSceneLoad<Game>();
    if (input.IsKeyPressed(GLFW_KEY_ESCAPE))
        RR::Engine::GetInstance().SetShouldClose(true);

    // FPS
    m_fpsTimer   += _deltaTime;
    m_frameCount += 1;
    if (m_fpsTimer >= 1.0f)
    {
        const int numPits = kPitsPerSide * kPitsPerSide;
        RR::Log("[MULTIPIT] pits=", numPits,
                " | ", m_frameCount / m_fpsTimer, " FPS",
                " | ", 1000.0f * m_fpsTimer / m_frameCount, " ms/frame",
                " | ", m_boxCount, " boxes");
        m_fpsTimer = 0.0f; m_frameCount = 0;
    }

    // Rain round-robin across pits, up to the constant total
    const int numPits = static_cast<int>(m_pitCenters.size());
    const float interval = 0.04f;
    const int   batch    = 6;
    m_spawnTimer += _deltaTime;
    while (m_spawnTimer >= interval && m_boxCount < kTotalBoxes)
    {
        m_spawnTimer -= interval;
        for (int b = 0; b < batch && m_boxCount < kTotalBoxes; ++b)
        {
            const vec3  c  = m_pitCenters[m_boxCount % numPits];
            const float jx = std::sin(m_boxCount * 78.233f)  * (m_pitHalf * 0.7f);
            const float jz = std::cos(m_boxCount * 12.9898f) * (m_pitHalf * 0.7f);
            const quat  rot(vec3(m_boxCount * 0.11f, m_boxCount * 0.07f, m_boxCount * 0.13f));
            SpawnBox(c + vec3(jx, 18.0f, jz), rot);
        }
    }
}

void MultiPitBenchmark::LateUpdate(float) {}
void MultiPitBenchmark::Destroy()      { RR::Log("[MULTIPIT] destroyed."); }