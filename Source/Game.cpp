
#include "Game.h"

#include "TestObject.h"

// PUBLIC --------------------------------------------------------------------------------------------------------------

Game::Game() = default;

Game::~Game() = default;

bool Game::Init()
{
    m_scene = new RR::Scene;

    // Create scene camera
    auto camera = m_scene->CreateObject("Camera");
    camera->AddComponent(new RR::CameraComponent());
    camera->SetPosition(Vec3(0.0f, 0.0f, 2.0f));
    camera->AddComponent(new RR::PlayerControllerComponent);

    m_scene->SetMainCamera(camera);
    //m_scene->CreateObject<TestObject>("TestObject");

    const std::string vertPath = std::string(ASSETS_PATH) + "Shaders/basic.vert";
    const std::string fragPath = std::string(ASSETS_PATH) + "Shaders/basic.frag";

    auto& graphicsAPI = RR::Engine::GetInstance().GetGraphicsAPI();

    auto shaderProgram = graphicsAPI.CreateShaderProgram(vertPath, fragPath);

    auto material = std::make_shared<RR::Material>();
    material->SetShaderProgram(shaderProgram);

    // Triangle made with vertices
    // Color of each vertices is on the right
    std::vector<float> vertices
    {
        // Front face
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f,

        // Back face
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f
    };

    // Instead of re-rendering vertices, use them from the existent array
    std::vector<unsigned int> indeces =
    {
        // front face
        0, 1, 2,
        0, 2, 3,
        // top face
        4, 5, 1,
        4, 1, 0,
        // right face
        4, 0, 3,
        4, 3, 7,
        //left face
        1, 5, 6,
        1, 6, 2,
        // bottom face
        3, 2, 6,
        3, 6, 7,
        // back face
        4, 7, 6,
        4, 6, 5
    };

    RR::VertexLayout vertexLayout;

    // Position
    vertexLayout.elements.push_back({
        0,
        3,
        GL_FLOAT,
        0
    });

    // Color
    vertexLayout.elements.push_back({
        1,
        3,
        GL_FLOAT,
        sizeof(float) * 3
        });

    // Stride
    vertexLayout.stride = sizeof(float) * 6;

    auto mesh = std::make_shared<RR::Mesh>(vertexLayout, vertices, indeces);

    auto objectA = m_scene->CreateObject("ObjectA");
    objectA->AddComponent(new RR::MeshComponent(material, mesh));
    objectA->SetPosition(Vec3(0.0f, 2.0f, 0.0f));

    auto objectB = m_scene->CreateObject("ObjectB");
    objectB->AddComponent(new RR::MeshComponent(material, mesh));
    objectB->SetPosition(Vec3(0.0f, 2.0f, 2.0f));
    objectB->SetRotation(Vec3(0.0f, 2.0f, 0.0f));

    auto objectC = m_scene->CreateObject("ObjectC");
    objectC->AddComponent(new RR::MeshComponent(material, mesh));
    objectC->SetPosition(Vec3(-2.0f, 0.0f, 0.0f));
    objectC->SetRotation(Vec3(1.0f, 0.0f, 1.0f));
    objectC->SetScale(Vec3(1.5f));



    RR::Engine::GetInstance().SetScene(m_scene);

    return true;
}

void Game::Update(float _deltaTime)
{
    m_scene->Update(_deltaTime);
}

void Game::Destroy()
{
}
