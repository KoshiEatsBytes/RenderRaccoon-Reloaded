
#include "../Bullet.h"

void Bullet::Init()
{
    GameObject::Init();


    static std::shared_ptr<RR::Material> s_material = RR::Material::Load("Materials/Brick.json");
    static std::shared_ptr<RR::Mesh>     s_mesh     = RR::Mesh::CreateSphere(0.2f, 32, 32);

    AddComponent<RR::MeshComponent>(s_material, s_mesh);

    auto collider = AddComponent<RR::ColliderComponent>(std::make_shared<RR::SphereCollider>(0.2f));
    m_physics = AddComponent<RR::PhysicsComponent>(RR::BodyType::DYNAMIC, 10.f, 0.1f);
}

void Bullet::Update(float _deltaTime)
{
    GameObject::Update(_deltaTime);

    m_lifetime -= _deltaTime;

    if (m_lifetime <= 0)
        MarkForDestroy();
}

void Bullet::Teleport(const vec3& _dir)
{
    m_physics->Teleport(_dir);
}

void Bullet::ApplyCentralImpulse(const vec3 &_direction)
{
    m_physics->ApplyCentralImpulse(_direction);
}
