
#include "PlayerControllerComponent.h"

#include "Engine.h"
#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    PlayerControllerComponent::PlayerControllerComponent()
    = default;

    PlayerControllerComponent::~PlayerControllerComponent()
    {
        if (m_owner) m_owner->m_physicsOwnership = PhysicsOwnership::NONE;
    }

    void PlayerControllerComponent::Init()
    {
        Component::Init();

        m_kinematicController = std::make_unique<KinematicCharacterController>(m_capsuleRadius, m_capsuleHeight);

        // Inherits from the GO just at start
        m_kinematicController->SetPosition(m_owner->GetWorldPosition());

        m_owner->m_physicsOwnership = PhysicsOwnership::CHARACTER;
    }

    void PlayerControllerComponent::Update(float _deltaTime)
    {
        auto& inputManager = Engine::GetInstance().GetInputManager();
        auto rotation = m_owner->GetRotation();

        // rotate around Y axis for horizontal movement and around X for vertical
        if (inputManager.GetMousePositionChanged())
        {
            const auto& oldPos = inputManager.GetMousePositionOld();
            const auto& currPos = inputManager.GetMousePositionCurrent();

            float deltaX = currPos.x - oldPos.x;
            float deltaY = currPos.y - oldPos.y;

            // horizontal rot
            float yDeltaAngle = -deltaX * m_sensitivity * _deltaTime;
            m_yRot += yDeltaAngle;
            m_yRot = std::fmod(m_yRot, 360.0f);
            quat yRot = glm::angleAxis(glm::radians(m_yRot), vec3(0.0f, 1.0f, 0.0f));

            // vertical rot
            float xDeltaAngle = -deltaY * m_sensitivity * _deltaTime;
            m_xRot += xDeltaAngle;
            m_xRot = std::clamp(m_xRot, m_verticalRotConstraints.x, m_verticalRotConstraints.y);
            quat xRot = glm::angleAxis(glm::radians(m_xRot), vec3(1.0f, 0.0f, 0.0f));

            rotation = glm::normalize(yRot * xRot);

            // set rot to owner GO
            m_owner->SetWorldRotationInternal(rotation);
        }

        // forward along z axis, right x
        quat yawOnly = glm::angleAxis(glm::radians(m_yRot), vec3(0.0f, 1.0f, 0.0f));
        vec3 forward = yawOnly * vec3(0.0f, 0.0f, -1.0f);
        vec3 right   = yawOnly * vec3(1.0f, 0.0f, 0.0f);
        vec3 move    {0.0f};

        // Left/Right
        if (inputManager.IsKeyPressed(GLFW_KEY_A))
        {
            move -= right;
        }
        if (inputManager.IsKeyPressed(GLFW_KEY_D))
        {
            move += right;
        }
        // Up/Down
        if (inputManager.IsKeyPressed(GLFW_KEY_W))
        {
            move += forward;
        }
        if (inputManager.IsKeyPressed(GLFW_KEY_S))
        {
            move -= forward;
        }

        if (inputManager.IsKeyPressed(GLFW_KEY_SPACE))
        {
            m_kinematicController->Jump(m_jumpTrajectory);
        }

        // check if move vector has any input applied
        if (glm::dot(move, move) > 0)
        {
            move = glm::normalize(move);
        }

        m_kinematicController->SetWalkVelocity(move * m_moveSpeed, _deltaTime);

        vec3 eyeOffset(0.0f, m_capsuleHeight * 0.5f + m_capsuleRadius * 0.5f, 0.0f);
        m_owner->SetWorldPositionInternal(m_kinematicController->GetPosition() + eyeOffset);
    }

    void PlayerControllerComponent::Teleport(const vec3& _worldPos)
    {
        if (m_kinematicController) m_kinematicController->SetPosition(_worldPos);
    }

    void PlayerControllerComponent::SetLookRotation(float _yawDegrees, float _pitchDegrees)
    {
        // Overrides internal rotation for character controller
        m_yRot = std::fmod(_yawDegrees, 360.0f);
        m_xRot = std::clamp(_pitchDegrees, m_verticalRotConstraints.x, m_verticalRotConstraints.y);
    }

    vec2 PlayerControllerComponent::GetLookRotation() const
    {
        return vec2{m_yRot, m_xRot};
    }

    void PlayerControllerComponent::SetMoveSpeed(float _speed)
    {
        m_moveSpeed = _speed;
    }

    void PlayerControllerComponent::SetMouseSensitivity(float _sens)
    {
        m_sensitivity = _sens;
    }

    float PlayerControllerComponent::GetMoveSpeed() const
    {
        return m_moveSpeed;
    }

    float PlayerControllerComponent::GetMouseSensitivity() const
    {
        return m_sensitivity;
    }
}
