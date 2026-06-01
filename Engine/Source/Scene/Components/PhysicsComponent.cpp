
#include "PhysicsComponent.h"

#include "Engine.h"
#include "Scene/GameObject.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    PhysicsComponent::PhysicsComponent()
    = default;

    PhysicsComponent::PhysicsComponent(const std::shared_ptr<RigidBody>& _body)
        : m_rigidBody(_body)
    {
    }

    PhysicsComponent::~PhysicsComponent()
    = default;

    void PhysicsComponent::Init()
    {
        if (!m_rigidBody)
        {
            return;
        }

        const auto pos = m_owner->GetWorldPosition();
        const auto rot = m_owner->GetRotation();

        m_rigidBody->SetPosition(pos);
        m_rigidBody->SetRotation(rot);

        Engine::GetInstance().GetPhysicsManager().AddRigidbody(m_rigidBody.get());
    }

    void PhysicsComponent::Update(float _deltaTime)
    {
        if (!m_rigidBody)
        {
            return;
        }

        // Only update if dynamic 
        if (m_rigidBody->GetType() == BodyType::DYNAMIC)
        {
            m_owner->SetPosition(m_rigidBody->GetPosition());
            m_owner->SetRotation(m_rigidBody->GetRotation());
        }
    }
}
