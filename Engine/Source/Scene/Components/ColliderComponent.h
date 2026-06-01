
#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include "Physics/Collider.h"
#include "Scene/Component.h"

namespace RR
{
    class ColliderComponent : public Component
    {
    public:
        COMPONENT(ColliderComponent)

        ColliderComponent(const std::shared_ptr<Collider>& _col);
        ~ColliderComponent() override;

        void Init() override;
        void Update(float _deltaTime) override;

        Collider* GetCollider() const;
        std::shared_ptr<Collider> GetColliderShared() const;

    private:
        std::shared_ptr<Collider> m_collider;
    };
}


