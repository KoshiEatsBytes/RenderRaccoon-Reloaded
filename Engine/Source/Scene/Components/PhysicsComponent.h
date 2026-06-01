#pragma once

#include "Scene/Component.h"
#include "Physics/RigidBody.h"

namespace RR
{
    class PhysicsComponent : public Component
    {
    public:
        COMPONENT(PhysicsComponent)

        PhysicsComponent();
        PhysicsComponent(const std::shared_ptr<RigidBody>& _body);
        ~PhysicsComponent() override;

        void Init() override;
        void Update(float _deltaTime) override;

    private:
        std::shared_ptr<RigidBody> m_rigidBody;
    };
}