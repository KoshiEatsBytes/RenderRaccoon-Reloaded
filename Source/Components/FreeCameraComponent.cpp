
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
        ApplyIntent(GatherLiveIntent(), _deltaTime);
    }

    void FreeCameraComponent::ApplyIntent(const CameraIntent& _in, float _deltaTime)
    {
        // sensitivity
        if (_in.sensScroll != 0.0f)
        {
            float step = _in.sensScroll > 0 ? m_sensStep : m_sensStep * -1.f;
            m_sens = std::clamp(m_sens + step, m_sensMin, m_sensMax);
        }

        // yaw
        m_yaw += -_in.lookDeltaX * m_sens;
        m_yaw  = std::fmod(m_yaw, 360.0f);
        // pitch
        m_pitch += -_in.lookDeltaY * m_sens;
        m_pitch  = std::clamp(m_pitch, m_verticalRotConstraints.x, m_verticalRotConstraints.y);

        // calculate orientation with pitch and roll
        quat yaw = glm::angleAxis(glm::radians(m_yaw), vec3(0.0f, 1.0f, 0.0f));
        quat pitch = glm::angleAxis(glm::radians(m_pitch), vec3(1.0f, 0.0f, 0.0f));
        m_orientation = glm::normalize(yaw * pitch);
        m_owner->SetWorldRotation(m_orientation);

        // Movement intent, all axis
        vec3 forward = m_orientation * vec3(0,0,-1);
        vec3 right   = m_orientation * vec3(1,0,0);
        vec3 up      = vec3(0,1,0);
        vec3 move    = right * _in.moveAxis.x + up * _in.moveAxis.y + forward * _in.moveAxis.z;

        if (glm::dot(move, move) > 0.0f)
        {
            float speed = !_in.sprint ? m_moveSpeed : m_sprintSpeed;

            m_owner->SetWorldPosition(
                m_owner->GetWorldPosition() + glm::normalize(move) * speed * _deltaTime);
        }
    }

    CameraIntent FreeCameraComponent::GatherLiveIntent() const
    {
        CameraIntent intent;

        // Camera rotation
        if (m_inputManager.GetMousePositionChanged())
        {
            const auto& oldPos  = m_inputManager.GetMousePositionOld();
            const auto& currPos = m_inputManager.GetMousePositionCurrent();
            intent.lookDeltaX = currPos.x - oldPos.x;
            intent.lookDeltaY = currPos.y - oldPos.y;
        }

        intent.sensScroll = m_inputManager.GetScrollDelta();

        auto Pressed = [&](int _key) {
            return m_inputManager.IsKeyPressed(_key);
        };

        // movement vector
        if (Pressed(GLFW_KEY_W) ||
            Pressed(GLFW_KEY_UP))
            intent.moveAxis.z += 1.0f;

        if (Pressed(GLFW_KEY_S) ||
            Pressed(GLFW_KEY_DOWN))
            intent.moveAxis.z -= 1.0f;

        if (Pressed(GLFW_KEY_D) ||
            Pressed(GLFW_KEY_RIGHT))
            intent.moveAxis.x += 1.0f;

        if (Pressed(GLFW_KEY_A) ||
            Pressed(GLFW_KEY_LEFT))
            intent.moveAxis.x -= 1.0f;

        if (Pressed(GLFW_KEY_E) ||
            Pressed(GLFW_KEY_SPACE))
            intent.moveAxis.y += 1.0f;

        if (Pressed(GLFW_KEY_LEFT_CONTROL) ||
            Pressed(GLFW_KEY_LEFT_ALT)     ||
            Pressed(GLFW_KEY_Q))
            intent.moveAxis.y -= 1.0f;

        if (Pressed(GLFW_KEY_LEFT_SHIFT))
            intent.sprint = true;

        return intent;
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
