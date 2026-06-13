
#include "FreeCameraComponent.h"
#include "Engine.h"
#include "GLFW/glfw3.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    FreeCameraComponent::FreeCameraComponent()
        : m_inputManager(Engine::GetInstance().GetInputManager()) {}

    FreeCameraComponent::~FreeCameraComponent()
    = default;

    void FreeCameraComponent::Init()
    {
        SetFOV(75.f);
    }

    void FreeCameraComponent::PreUpdate(float _deltaTime)
    {
        // Camera rotation
        if (m_inputManager.GetMousePositionChanged())
        {
            const auto& oldPos  = m_inputManager.GetMousePositionOld();
            const auto& currPos = m_inputManager.GetMousePositionCurrent();
            float deltaX = currPos.x - oldPos.x;
            float deltaY = currPos.y - oldPos.y;

            m_yaw += -deltaX * m_sens;
            m_yaw  = std::fmod(m_yaw, 360.0f);

            m_pitch += -deltaY * m_sens;
            m_pitch  = std::clamp(m_pitch, m_verticalRotConstraints.x, m_verticalRotConstraints.y);
        }

        // calculate orientation with pitch and roll
        quat yaw = glm::angleAxis(glm::radians(m_yaw), vec3(0.0f, 1.0f, 0.0f));
        quat pitch = glm::angleAxis(glm::radians(m_pitch), vec3(1.0f, 0.0f, 0.0f));
        m_orientation = glm::normalize(yaw * pitch);
        m_owner->SetWorldRotation(m_orientation);

        // mouse wheel bound sensitivity
        float scroll = m_inputManager.GetScrollDelta();
        if (scroll != 0.0f)
        {
           Log(scroll);
        }

        // Movement intent, all axis
        vec3 forward = m_orientation * vec3(0,0,-1);
        vec3 right   = m_orientation * vec3(1,0,0);
        vec3 up      = vec3(0,1,0);

        vec3 move {0};
        bool sprint = false;

        if (m_inputManager.IsKeyPressed(GLFW_KEY_W) ||
            m_inputManager.IsKeyPressed(GLFW_KEY_UP))
            move += forward;

        if (m_inputManager.IsKeyPressed(GLFW_KEY_S) ||
            m_inputManager.IsKeyPressed(GLFW_KEY_DOWN))
            move -= forward;

        if (m_inputManager.IsKeyPressed(GLFW_KEY_D) ||
            m_inputManager.IsKeyPressed(GLFW_KEY_RIGHT))
            move += right;

        if (m_inputManager.IsKeyPressed(GLFW_KEY_A) ||
            m_inputManager.IsKeyPressed(GLFW_KEY_LEFT))
            move -= right;

        if (m_inputManager.IsKeyPressed(GLFW_KEY_SPACE) ||
            m_inputManager.IsKeyPressed(GLFW_KEY_E))
            move += up;

        if (m_inputManager.IsKeyPressed(GLFW_KEY_LEFT_CONTROL) ||
            m_inputManager.IsKeyPressed(GLFW_KEY_LEFT_ALT)     ||
            m_inputManager.IsKeyPressed(GLFW_KEY_Q))
            move -= up;

        if (m_inputManager.IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
            sprint = true;

        if (glm::dot(move, move) > 0.0f)
        {
            float speed = !sprint ? m_moveSpeed : m_sprintSpeed;

            m_owner->SetWorldPosition(
                m_owner->GetWorldPosition() + glm::normalize(move) * speed * _deltaTime);
        }
    }

    quat FreeCameraComponent::GetOrientation() const
    {
        return m_orientation;
    }

    float FreeCameraComponent::GetPitch() const
    {
        return m_pitch;
    }

    float FreeCameraComponent::GetYaw() const
    {
        return m_yaw;
    }

    void FreeCameraComponent::SetPitch(float _pitch)
    {
        m_pitch = std::clamp(_pitch, m_verticalRotConstraints.x, m_verticalRotConstraints.y);
    }

    void FreeCameraComponent::SetYaw(float _yaw)
    {
        m_yaw = std::fmod(_yaw, 360.0f);;
    }
}
