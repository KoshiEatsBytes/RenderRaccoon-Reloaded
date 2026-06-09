#include "Benchmark.h"
#include "Game.h"
#include "GLFW/glfw3.h"
#include <cmath>

#include "Benchmark/BenchmarkSubSystem.h"

Benchmark::Benchmark() : Scene("BenchmarkScene") {}

bool Benchmark::Init()
{
    SetCursorEnabled(false);

    m_material = RR::Material::Load("Materials/Brick.json");
    m_boxMesh  = RR::Mesh::CreateBox(vec3(1.0f));

    auto camera = CreateObject("Camera");
    camera->AddComponent(new RR::CameraComponent());
    camera->SetPosition(vec3(0.0f, 36.0f, 30.0f));
    camera->SetRotation(quat(vec3(-0.87f, 0.0f, 0.0f)));   // ~-50° pitch
    SetMainCamera(camera);

    auto light = CreateObject("Light");
    auto lightComp = new RR::LightComponent();
    lightComp->SetColor(vec3(1.0f));
    light->AddComponent(lightComp);
    light->SetPosition(vec3(0.0f, 35.0f, 10.0f));

    {
        const vec3 ext(60.0f, 2.0f, 60.0f);
        auto ground = CreateObject("Ground");
        ground->SetPosition(vec3(0.0f, -5.0f, 0.0f));
        ground->AddComponent(new RR::MeshComponent(m_material, RR::Mesh::CreateBox(ext)));
        ground->AddComponent(new RR::ColliderComponent(std::make_shared<RR::BoxCollider>(ext)));
        ground->AddComponent(new RR::PhysicsComponent(RR::BodyType::STATIC, 0.0f, 0.8f));
    }

    auto makeWall = [&](const std::string& _name, const vec3& _pos, const vec3& _ext)
    {
        auto wall = CreateObject(_name);
        wall->SetPosition(_pos);
        wall->AddComponent(new RR::MeshComponent(m_material, RR::Mesh::CreateBox(_ext)));
        wall->AddComponent(new RR::ColliderComponent(std::make_shared<RR::BoxCollider>(_ext)));
        wall->AddComponent(new RR::PhysicsComponent(RR::BodyType::STATIC, 0.0f, 0.5f));
    };
    const float pit = 12.0f, wallH = 16.0f;
    makeWall("WallN", vec3(0.0f, 0.0f, -pit), vec3(pit * 2.0f, wallH, 1.0f));
    makeWall("WallS", vec3(0.0f, 0.0f,  pit), vec3(pit * 2.0f, wallH, 1.0f));
    makeWall("WallE", vec3( pit, 0.0f, 0.0f), vec3(1.0f, wallH, pit * 2.0f));
    makeWall("WallW", vec3(-pit, 0.0f, 0.0f), vec3(1.0f, wallH, pit * 2.0f));

    RR::Success("[BENCHMARK] Arena ready — raining in up to ", kMaxBoxes, " boxes.");

    m_bench = RR::Engine::GetInstance().GetAppManager().GetSubSystem<RR::BenchmarkSubSystem>();

    return true;
}

void Benchmark::SpawnBox(const vec3& _pos, const quat& _rot)
{
    auto box = CreateObject("Box");
    box->SetPosition(_pos);
    box->SetRotation(_rot);
    box->AddComponent(new RR::MeshComponent(m_material, m_boxMesh));
    box->AddComponent(new RR::ColliderComponent(std::make_shared<RR::BoxCollider>(vec3(1.0f))));
    box->AddComponent(new RR::PhysicsComponent(RR::BodyType::DYNAMIC, 1.0f, 0.5f));
    m_spawnedBoxes.push_back(box);
    ++m_boxCount;
}

void Benchmark::PreUpdate(float) {}

void Benchmark::Update(float _deltaTime)
{
    auto& input = RR::Engine::GetInstance().GetInputManager();

    if (input.IsKeyPressed(GLFW_KEY_1))
        RR::Engine::GetInstance().GetAppManager().RequestSceneLoad<Game>();
    if (input.IsKeyPressed(GLFW_KEY_ESCAPE))
        RR::Engine::GetInstance().SetShouldClose(true);
    if (input.IsKeyPressed(GLFW_KEY_K))
        m_spawn = false;
    if (input.IsKeyPressed(GLFW_KEY_J))
        m_spawn = true;

    if (input.IsKeyPressed(GLFW_KEY_N))
    {
        if (m_bench->IsLogging())
        {
            m_bench->RequestStopLogging();
        }
    }
    if (input.IsKeyPressed(GLFW_KEY_M))
    {
        if (!m_bench->IsLogging())
        {
            RR::RunInfo info;

            info.scenario   = "TestBench";
            info.seed       = 0;
            info.lod        = false;
            info.async      = false;
            info.scheduling = false;
            info.lodCache   = false;
            info.greedy     = false;

            m_bench->RequestStartLogging(info);
        }
    }

    // avg fps in 1 sec window
    m_fpsTimer   += _deltaTime;
    m_frameCount += 1;
    if (m_fpsTimer >= 1.0f)
    {
        const float fps = m_frameCount / m_fpsTimer;
        const float ms  = 1000.0f * m_fpsTimer / m_frameCount;
        RR::Log("[BENCHMARK] ", fps, " FPS | ", ms, " ms/frame | ", m_boxCount, " boxes");
        m_fpsTimer   = 0.0f;
        m_frameCount = 0;
    }

    if (!m_spawn)
    {
        // Rain around 100 boxes/sec up to the cap
        const float interval = 0.04f;
        const int   batch    = 4;
        m_despawnTimer += _deltaTime;
        while (m_despawnTimer >= interval && m_boxCount > 0)
        {
            m_despawnTimer -= interval;
            for (int i = 0; i < batch && m_boxCount > 0; ++i)
            {
                m_spawnedBoxes.back()->MarkForDestroy();
                m_spawnedBoxes.pop_back();
                --m_boxCount;
            }
        }
    }
    else
    {
        // Rain around 100 boxes/sec up to the cap
        const float interval = 0.04f;
        const int   batch    = 4;
        m_spawnTimer += _deltaTime;
        while (m_spawnTimer >= interval && m_boxCount < kMaxBoxes)
        {
            m_spawnTimer -= interval;
            for (int i = 0; i < batch && m_boxCount < kMaxBoxes; ++i)
            {
                const float fx = std::sin(m_boxCount * 78.233f) * 9.0f;
                const float fz = std::cos(m_boxCount * 12.9898f) * 9.0f;
                const quat  rot(vec3(m_boxCount * 0.11f, m_boxCount * 0.07f, m_boxCount * 0.13f));
                SpawnBox(vec3(fx, 22.0f, fz), rot);
            }
        }
    }
}

void Benchmark::LateUpdate(float) {}

void Benchmark::Destroy()
{
    RR::Log("[BENCHMARK] Scene destroyed (", m_boxCount, " boxes).");
}