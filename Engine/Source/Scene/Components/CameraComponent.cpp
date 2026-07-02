
#include "CameraComponent.h"

#include "Engine.h"
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

    // Get/Sets --------------------------------------------------------------------------------------------------------

    mat4 CameraComponent::GetViewMatrix() const
    {
        mat4 mat = mat4(1.0f);

        // Transformation order does not follow MVP - dont apply scale
        mat = glm::mat4_cast(m_owner->GetRotation());
        mat[3] = vec4(m_owner->GetPosition(), 1.0f);

        // Apply parent transform is present
        if (m_owner->GetParent())
        {
            mat = m_owner->GetParent()->GetWorldTransform() * mat;
        }

        return glm::inverse(mat);
    }

    mat4 CameraComponent::GetProjectionMatrix(float _aspect) const
    {
        const float fovY = glm::radians(m_fov);

        if (Engine::GetInstance().GetGraphicsAPI().IsReversedZ())
        {
            // if reverse z far and near are swapped
            return glm::perspectiveRH_ZO(fovY, _aspect, m_farPlane, m_nearPlane);
        }

        return glm::perspective(fovY, _aspect, m_nearPlane, m_farPlane);
    }

    void CameraComponent::SetParameters(const float _fov, const float _nearPlane, const float _farPlane)
    {
        m_fov = _fov;
        m_nearPlane = _nearPlane;
        m_farPlane = _farPlane;
    }

    void CameraComponent::SetFOV(float _fov)
    {
        m_fov = _fov;
    }

    void CameraComponent::SetNearPlane(float _nearPlane)
    {
        m_nearPlane = _nearPlane;
    }

    void CameraComponent::SetFarPlane(float _farPlane)
    {
        m_farPlane = _farPlane;
    }

    float CameraComponent::GetFov() const
    {
        return m_fov;
    }

    float CameraComponent::GetNearPlane() const
    {
        return m_nearPlane;
    }

    float CameraComponent::GetFarPlane() const
    {
        return m_farPlane;
    }
}
