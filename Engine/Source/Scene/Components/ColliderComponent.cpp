
#include "ColliderComponent.h"

#include "PhysicsComponent.h"
#include "Scene/GameObject.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    ColliderComponent::ColliderComponent()
    = default;

    ColliderComponent::ColliderComponent(const std::shared_ptr<Collider>& _col)
        : m_collider(_col)
    {
    }

    ColliderComponent::~ColliderComponent()
    = default;

    void ColliderComponent::Init()
    {
        Component::Init();
    }

    // Colliders are passive
    void ColliderComponent::Update(float _deltaTime) {}

    void ColliderComponent::SetCollider(const std::shared_ptr<Collider>& _col)
    {
        m_collider = _col;

        // If part of a live body, rebuilt with new collider
        if (PhysicsComponent* pc = m_owner->FindComponentByType<PhysicsComponent>())
        {
            pc->Rebuild();
        }
    }

    std::shared_ptr<Collider> ColliderComponent::GetCollider() const
    {
        return m_collider;
    }
}
