
#include "ColliderComponent.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

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

    Collider* ColliderComponent::GetCollider() const
    {
        return m_collider.get();
    }

    std::shared_ptr<Collider> ColliderComponent::GetColliderShared() const
    {
        return m_collider;
    }
}
