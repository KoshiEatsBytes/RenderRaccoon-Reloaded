
#pragma once
#include "RR.h"
#include "Scene/GameObject.h"

class Bullet : public RR::GameObject
{
public:
    GAMEOBJECT(Bullet, RR::GameObject)

    void Init() override;

    void Update(float _deltaTime) override;

    void Teleport(const vec3& _dir);
    void ApplyCentralImpulse(const vec3& _direction);

private:
    RR::PhysicsComponent* m_physics = nullptr;
    float m_lifetime = 2.0f;
};

