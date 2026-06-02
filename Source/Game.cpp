

#include "Game.h"

#include "../Engine/Source/Helpers/GLTFLib.hpp"
#include "TestObject.h"
#include "Physics/Collider.h"
#include "Physics/RigidBody.h"
#include "Scene/Components/PhysicsComponent.h"

// PUBLIC --------------------------------------------------------------------------------------------------------------

Game::Game() = default;

Game::~Game() = default;

bool Game::Init()
{
    m_scene = new RR::Scene;
    RR::Engine::GetInstance().SetScene(m_scene);

    // Create scene camera
    auto camera = m_scene->CreateObject("Camera");
    camera->AddComponent(new RR::CameraComponent());
    camera->SetPosition(vec3(0.0f, 0.0f, 2.0f));
    camera->AddComponent(new RR::PlayerControllerComponent);

    m_scene->SetMainCamera(camera);
    //m_scene->CreateObject<TestObject>("TestObject");

    auto material = RR::Material::Load("Materials/Brick.json");
    auto mesh = RR::Mesh::CreateBox();

    auto objectA = m_scene->CreateObject("ObjectA");
    objectA->AddComponent(new RR::MeshComponent(material, mesh));
    objectA->SetPosition(vec3(1.0f, 0.0f, -15.0f));

    auto objectB = m_scene->CreateObject("ObjectB");
    objectB->AddComponent(new RR::MeshComponent(material, mesh));
    objectB->SetPosition(vec3(0.0f, 2.0f, 2.0f));
    objectB->SetRotation(vec3(0.0f, 2.0f, 0.0f));

    auto objectC = m_scene->CreateObject("ObjectC");
    objectC->AddComponent(new RR::MeshComponent(material, mesh));
    objectC->SetPosition(vec3(-2.0f, 0.0f, 0.0f));
    objectC->SetRotation(vec3(1.0f, 0.0f, 1.0f));
    objectC->SetScale(vec3(1.5f));

    //auto suzanneMesh = RR::Mesh::LoadGLTF("Models/Suzanne/Suzanne.gltf");
    //auto suzanneMat = RR::Material::Load("Materials/Suzanne.json");

    //auto suzanneObj = m_scene->CreateObject("Suzanne");
    //suzanneObj->AddComponent(new RR::MeshComponent(suzanneMat, suzanneMesh));
    //suzanneObj->SetPosition(vec3(-2.0f, 0.0f, -5.0f));

    //auto suzanneObj = RR::CGLTFLib::LoadGLTF("Models/Tardis/Tardis Exterior.gltf");
    auto suzanneObj = RR::CGLTFLib::LoadGLTF("Models/Suzanne/Suzanne.gltf");
    suzanneObj->SetPosition(vec3(0.0f, 0.0f, -5.0f));

    auto gun = RR::GameObject::LoadGLTF("Models/Gun/scene.gltf");
    gun->SetParent(camera);
    gun->SetPosition(vec3(0.75f, -0.5f, -0.75f));
    gun->SetScale(vec3(-1.0f, 1.0f, 1.0f));

    if (auto anim = gun->GetComponent<RR::AnimationComponent>())
    {
        if (auto bullet = gun->GetChildByName("bullet_33"))
        {
            bullet->SetActive(false);
        }

        if (auto flash = gun->GetChildByName("BOOM_35"))
        {
            flash->SetActive(false);
        }

        anim->Play("shoot", false);
    }


    auto light = m_scene->CreateObject("Light");
    auto lightComponent = new RR::LightComponent();
    lightComponent->SetColor(vec3(1.0f));
    light->AddComponent(lightComponent);
    light->SetPosition(vec3(0.0f, 5.0f, 0.0f));

    auto ground = m_scene->CreateObject("Ground");
    ground->SetPosition(vec3(0.0f, -5.0f, 0.0f));
    vec3 groundExtents (20.f, 2.f, 20.f);
    auto groundMesh = RR::Mesh::CreateBox(groundExtents);
    ground->AddComponent(new RR::MeshComponent(material, groundMesh));
    ground->AddComponent(new RR::ColliderComponent(std::make_shared<RR::BoxCollider>(groundExtents)));
    ground->AddComponent(new RR::PhysicsComponent(RR::BodyType::STATIC, 0.0f, 0.8f));

    auto boxObj = m_scene->CreateObject("FallingBox");
    boxObj->AddComponent(new RR::MeshComponent(material, mesh));
    boxObj->SetPosition(vec3(0.0f, 2.0f, 2.0f));
    boxObj->SetRotation(quat(vec3(1.0f,2.0f,0.0f)));
    auto boxCollider = std::make_shared<RR::BoxCollider>(vec3(1.0f));
    boxObj->AddComponent(new RR::ColliderComponent(boxCollider));
    boxObj->AddComponent(new RR::PhysicsComponent(RR::BodyType::DYNAMIC, 5.0f, 0.5f));


    camera->SetPosition(vec3(0.0f, 1.0f, 7.0f));

    return true;
}

void Game::Update(float _deltaTime)
{
    m_scene->Update(_deltaTime);
}

void Game::Destroy()
{
}
