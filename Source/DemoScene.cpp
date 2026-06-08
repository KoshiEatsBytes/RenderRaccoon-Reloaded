
#include "DemoScene.h"

#include "Game.h"
#include "Player.h"
#include "GLFW/glfw3.h"

// PUBLIC --------------------------------------------------------------------------------------------------------------

DemoScene::DemoScene() : Scene("DemoScene") {}
DemoScene::~DemoScene() = default;

bool DemoScene::Init()
{
    // One shared checker material for the whole arena; per-object colour is a MeshComponent tint
    // (the Default shader multiplies the texture by uColor).
    m_material = RR::Material::Load("Materials/Checker.json");

    // PLAYER — the Player class already wires camera, controller, listener, audio (shoot/step/jump) and a gun.
    auto player = CreateObject<Player>("MainPlayer");
    if (auto* pcc = player->FindComponentByType<RR::PlayerControllerComponent>())
        pcc->Teleport(vec3(0.0f, 2.0f, -7.0f));
    SetMainCamera(player);

    // LIGHT
    auto light     = CreateObject("Light");
    auto lightComp = new RR::LightComponent();
    lightComp->SetColor(vec3(1.0f));
    light->AddComponent(lightComp);
    light->SetPosition(vec3(-2.0f, 5.0f, 2.0f));

    // ARENA SHELL (static)
    MakeStaticBox("Ground",    vec3(  0.0f, 0.0f,   0.0f), vec3(30.0f, 1.0f, 30.0f));
    MakeStaticBox("LeftWall",  vec3(-15.5f, 3.0f,   0.0f), vec3( 1.0f, 5.0f, 30.0f));
    MakeStaticBox("RightWall", vec3( 15.5f, 3.0f,   0.0f), vec3( 1.0f, 5.0f, 30.0f));
    MakeStaticBox("FrontWall", vec3(  0.0f, 3.0f, -15.5f), vec3(30.0f, 5.0f,  1.0f));
    MakeStaticBox("BackWall",  vec3(  0.0f, 3.0f,  15.5f), vec3(30.0f, 5.0f,  1.0f));

    // PLATFORMS + STEPPED LADDER (static)
    MakeStaticBox("PlatformA", vec3(-14.0f, 1.05f, -14.0f), vec3(2.0f, 1.1f, 2.0f));

    for (int i = 0; i < 9; ++i)
    {
        const float x = -12.85f + 0.30f * static_cast<float>(i);
        const float y =   1.35f - 0.10f * static_cast<float>(i);
        MakeStaticBox("LadderA_" + std::to_string(i + 1), vec3(x, y, -14.0f), vec3(0.3f, 0.1f, 2.0f));
    }

    MakeStaticBox("PlatformB", vec3(-14.0f, 1.05f, -10.0f), vec3(2.0f, 1.1f, 2.0f));
    MakeStaticBox("PlatformC", vec3(-14.0f, 1.05f,  -3.0f), vec3(2.0f, 1.1f, 8.0f)); // JSON name was Cyrillic "С"
    MakeStaticBox("PlatformD", vec3( -7.0f, 1.05f,  -3.0f), vec3(8.0f, 1.1f, 2.0f));

    MakeStaticBox("InternalWall1", vec3(-1.0f, 1.5f, -11.0f), vec3(1.0f, 2.0f, 8.0f));
    MakeStaticBox("InternalWall2", vec3( 1.5f, 1.5f,  -1.0f), vec3(1.0f, 2.0f, 8.0f));

    // Jump pad (red). Original JSON used a custom "JumpPlatform" type; here it is a plain static box.
    MakeStaticBox("JumpPlatform", vec3(-7.0f, 1.75f, 1.0f), vec3(2.0f, 0.2f, 2.0f), vec3(1.0f, 0.0f, 0.0f));

    // DYNAMIC CUBE STACK (15 x 1.5^3, mass 15) along x = 10
    struct Spawn { vec3 pos; vec3 color; };
    const Spawn cubes[] = {
        {{10.0f, 1.251f, 0.000f}, {0.0f, 1.0f, 0.0f}},
        {{10.0f, 1.251f, 1.510f}, {0.7f, 0.5f, 0.2f}},
        {{10.0f, 1.251f, 3.020f}, {0.2f, 0.5f, 0.1f}},
        {{10.0f, 1.251f, 4.530f}, {0.2f, 0.1f, 0.9f}},
        {{10.0f, 1.251f, 6.040f}, {1.0f, 0.0f, 1.0f}},
        {{10.0f, 1.251f, 7.550f}, {0.1f, 0.8f, 0.7f}},
        {{10.0f, 2.752f, 0.750f}, {0.1f, 0.8f, 0.7f}},
        {{10.0f, 2.752f, 2.251f}, {0.9f, 0.2f, 0.3f}},
        {{10.0f, 2.752f, 3.752f}, {0.2f, 0.2f, 0.3f}},
        {{10.0f, 2.752f, 5.253f}, {0.5f, 0.1f, 0.9f}},
        {{10.0f, 2.752f, 6.754f}, {1.0f, 1.0f, 0.0f}},
        {{10.0f, 4.253f, 1.500f}, {1.0f, 1.0f, 0.0f}},
        {{10.0f, 4.253f, 3.010f}, {1.0f, 0.0f, 0.0f}},
        {{10.0f, 4.253f, 4.520f}, {0.0f, 0.0f, 1.0f}},
        {{10.0f, 4.253f, 6.030f}, {0.0f, 1.0f, 1.0f}},
    };

    int n = 1;
    for (const auto& c : cubes)
        MakeDynamicBox("ObjectCollide" + std::to_string(n++), c.pos, vec3(1.5f), 15.0f, c.color);

    // DYNAMIC SPHERES (3 x r1.5, mass 15) along x = 10
    const Spawn balls[] = {
        {{10.0f, 1.251f, -8.0f}, {0.0f, 1.0f, 0.0f}},
        {{10.0f, 1.251f, -6.0f}, {0.0f, 0.0f, 1.0f}},
        {{10.0f, 1.251f, -4.0f}, {1.0f, 0.0f, 0.0f}},
    };
    for (const auto& b : balls)
        MakeDynamicSphere("ObjectCollide" + std::to_string(n++), b.pos, 1.5f, 15.0f, b.color);

    RR::Success("[DEMO SCENE] Arena loaded. ESC = quit, 0 = back to Game.");
    return true;
}

void DemoScene::PreUpdate(float) {}

void DemoScene::Update(float)
{
    auto& engine = RR::Engine::GetInstance();
    auto& input  = engine.GetInputManager();

    if (input.IsKeyPressed(GLFW_KEY_ESCAPE)) engine.SetShouldClose(true);
    if (input.IsKeyPressed(GLFW_KEY_0))      engine.GetAppManager().RequestSceneLoad<Game>();
}

void DemoScene::LateUpdate(float) {}
void DemoScene::Destroy() {}

// PRIVATE -------------------------------------------------------------------------------------------------------------

std::shared_ptr<RR::Mesh> DemoScene::BoxMesh(const vec3& _extents)
{
    for (auto& [ext, mesh] : m_boxMeshes)
        if (ext == _extents) return mesh;

    auto mesh = RR::Mesh::CreateBox(_extents);
    m_boxMeshes.emplace_back(_extents, mesh);
    return mesh;
}

RR::GameObject* DemoScene::MakeStaticBox(const std::string& _name, const vec3& _pos,
                                         const vec3& _extents, const vec3& _color)
{
    auto obj = CreateObject(_name);
    obj->SetPosition(_pos);

    auto mesh = new RR::MeshComponent(m_material, BoxMesh(_extents));
    mesh->SetColor(_color);
    obj->AddComponent(mesh);

    obj->AddComponent(new RR::ColliderComponent(std::make_shared<RR::BoxCollider>(_extents)));
    obj->AddComponent(new RR::PhysicsComponent(RR::BodyType::STATIC, 0.0f, 0.5f));
    return obj;
}

RR::GameObject* DemoScene::MakeDynamicBox(const std::string& _name, const vec3& _pos,
                                          const vec3& _extents, float _mass, const vec3& _color)
{
    auto obj = CreateObject(_name);
    obj->SetPosition(_pos);

    auto mesh = new RR::MeshComponent(m_material, BoxMesh(_extents));
    mesh->SetColor(_color);
    obj->AddComponent(mesh);

    obj->AddComponent(new RR::ColliderComponent(std::make_shared<RR::BoxCollider>(_extents)));
    obj->AddComponent(new RR::PhysicsComponent(RR::BodyType::DYNAMIC, _mass, 0.5f));
    return obj;
}

RR::GameObject* DemoScene::MakeDynamicSphere(const std::string& _name, const vec3& _pos,
                                             float _radius, float _mass, const vec3& _color)
{
    auto obj = CreateObject(_name);
    obj->SetPosition(_pos);

    auto mesh = new RR::MeshComponent(m_material, RR::Mesh::CreateSphere(_radius, 32, 32));
    mesh->SetColor(_color);
    obj->AddComponent(mesh);

    obj->AddComponent(new RR::ColliderComponent(std::make_shared<RR::SphereCollider>(_radius)));
    obj->AddComponent(new RR::PhysicsComponent(RR::BodyType::DYNAMIC, _mass, 0.5f));
    return obj;
}
