
#include <stb_image.h>

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
    camera->SetPosition(vec3(0.0f, 0.0f, 2.0f));
    camera->AddComponent(new RR::PlayerControllerComponent);

    m_scene->SetMainCamera(camera);
    //m_scene->CreateObject<TestObject>("TestObject");

    auto material = RR::Material::Load("Materials/Brick.json");

    // Triangle made with vertices
    // Color of each vertices is on the right
    std::vector<float> vertices
    {
        // Front face
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

        // Top face
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

        // Right face
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

        // Left face
        -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

        // Bottom face
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

        // Back face
        -0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f
    };

    // Instead of re-rendering vertices, use them from the existent array
    std::vector<unsigned int> indeces =
    {
        // front face
        0, 1, 2,
        0, 2, 3,
        // top face
        4, 5, 6,
        4, 6, 7,
        // right face
        8, 9, 10,
        8, 10, 11,
        //left face
        12, 13, 14,
        12, 14, 15,
        // bottom face
        16, 17, 18,
        16, 18, 19,
        // back face
        20, 21, 22,
        20, 22, 23
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

    // UV
    vertexLayout.elements.push_back({
        2,
        2,
        GL_FLOAT,
        sizeof(float) * 6
    });

    // Stride
    vertexLayout.stride = sizeof(float) * 8;

    auto mesh = std::make_shared<RR::Mesh>(vertexLayout, vertices, indeces);

    auto objectA = m_scene->CreateObject("ObjectA");
    objectA->AddComponent(new RR::MeshComponent(material, mesh));
    objectA->SetPosition(vec3(0.0f, 2.0f, 0.0f));

    auto objectB = m_scene->CreateObject("ObjectB");
    objectB->AddComponent(new RR::MeshComponent(material, mesh));
    objectB->SetPosition(vec3(0.0f, 2.0f, 2.0f));
    objectB->SetRotation(vec3(0.0f, 2.0f, 0.0f));

    auto objectC = m_scene->CreateObject("ObjectC");
    objectC->AddComponent(new RR::MeshComponent(material, mesh));
    objectC->SetPosition(vec3(-2.0f, 0.0f, 0.0f));
    objectC->SetRotation(vec3(1.0f, 0.0f, 1.0f));
    objectC->SetScale(vec3(1.5f));



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
