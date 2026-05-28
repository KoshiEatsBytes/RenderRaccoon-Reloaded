
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
        Mat4 mat = Mat4(1.0f);

        // Transformation order does not follow MVP - dont apply scale
        mat = glm::mat4_cast(m_owner->GetRotation());
        mat[3] = Vec4(m_owner->GetPosition(), 1.0f);

        // Apply parent transform is present
        if (m_owner->GetParent())
        {
            mat = m_owner->GetParent()->GetWorldTransform() * mat;
        }

        return glm::inverse(mat);
    }

    Mat4 CameraComponent::GetProjectionMatrix(float _aspect) const
    {
        // calculate projection matrix
        return glm::perspective(glm::radians(m_fov), _aspect, m_nearPlane, m_farPlane);
    }
}
