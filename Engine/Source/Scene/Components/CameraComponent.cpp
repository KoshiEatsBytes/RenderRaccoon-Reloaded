
#include "CameraComponent.h"
#include "Scene/GameObject.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    CameraComponent::CameraComponent()
    = default;

    CameraComponent::~CameraComponent()
    = default;

    void CameraComponent::Update(float _deltaTime)
    {
    }

    Mat4 CameraComponent::GetViewMatrix() const
    {
        // Camera has inverted matrix
        return glm::inverse(m_owner->GetWorldTransform());
    }

    Mat4 CameraComponent::GetProjectionMatrix() const
    {
        return Mat4(1.0f);
    }
}
