
#include "CameraComponent.h"

#include "glm/ext/matrix_clip_space.hpp"
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

    Mat4 CameraComponent::GetProjectionMatrix(float _aspect) const
    {
        // calculate projection matrix
        return glm::perspective(glm::radians(m_fov), _aspect, m_nearPlane, m_farPlane);
    }
}
