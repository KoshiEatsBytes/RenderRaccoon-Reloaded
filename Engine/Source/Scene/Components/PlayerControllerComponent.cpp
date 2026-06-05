
#include "PlayerControllerComponent.h"

#include "Engine.h"
#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Physics/PhysicsManager.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    PlayerControllerComponent::PlayerControllerComponent()
    = default;

    PlayerControllerComponent::PlayerControllerComponent(const float _capsuleHeight, const float _capsuleRadius)
        : m_capsuleHeight(_capsuleHeight), m_capsuleRadius(_capsuleRadius)
    {
    }

    PlayerControllerComponent::~PlayerControllerComponent()
    {
        if (m_owner) m_owner->m_physicsOwnership = PhysicsOwnership::NONE;
    }

    void PlayerControllerComponent::Init()
    {
        Component::Init();

        m_kinematicController = std::make_unique<KinematicCharacterController>(
            *m_owner->m_scene->GetPhysicsManager(), m_capsuleRadius, m_capsuleHeight);

        // Inherits from the GO just at start
        m_kinematicController->SetPosition(m_owner->GetWorldPosition());

        m_owner->m_physicsOwnership = PhysicsOwnership::CHARACTER;
    }

    void PlayerControllerComponent::PreUpdate(float _deltaTime)
    {
        auto& input = Engine::GetInstance().GetInputManager();

        // Mouse look → yaw/pitch → camera rotation
        if (input.GetMousePositionChanged())
        {
            const auto& oldPos  = input.GetMousePositionOld();
            const auto& currPos = input.GetMousePositionCurrent();
            float deltaX = currPos.x - oldPos.x;
            float deltaY = currPos.y - oldPos.y;

            m_yRot += -deltaX * m_sensitivity * _deltaTime;
            m_yRot  = std::fmod(m_yRot, 360.0f);

            m_xRot += -deltaY * m_sensitivity * _deltaTime;
            m_xRot  = std::clamp(m_xRot, m_verticalRotConstraints.x, m_verticalRotConstraints.y);

            quat yRot = glm::angleAxis(glm::radians(m_yRot), vec3(0.0f, 1.0f, 0.0f));
            quat xRot = glm::angleAxis(glm::radians(m_xRot), vec3(1.0f, 0.0f, 0.0f));
            m_owner->SetWorldRotationInternal(glm::normalize(yRot * xRot));
        }

        // Movement intent (yaw-only) walk velocity, fed to physics this step
        quat yawOnly = glm::angleAxis(glm::radians(m_yRot), vec3(0.0f, 1.0f, 0.0f));
        vec3 forward = yawOnly * vec3(0.0f, 0.0f, -1.0f);
        vec3 right   = yawOnly * vec3(1.0f, 0.0f, 0.0f);
        vec3 move{0.0f};

        if (input.IsKeyPressed(GLFW_KEY_A)) move -= right;
        if (input.IsKeyPressed(GLFW_KEY_D)) move += right;
        if (input.IsKeyPressed(GLFW_KEY_W)) move += forward;
        if (input.IsKeyPressed(GLFW_KEY_S)) move -= forward;

        if (input.IsKeyPressed(GLFW_KEY_SPACE))
            m_kinematicController->Jump(m_jumpTrajectory);

        if (glm::dot(move, move) > 0.0f)
            move = glm::normalize(move);

        m_kinematicController->SetWalkVelocity(move * m_moveSpeed);
    }

    void PlayerControllerComponent::Update(float _deltaTime)
    {
        vec3 eyeOffset(0.0f, m_capsuleHeight * 0.5f + m_capsuleRadius * 0.5f, 0.0f);
        float alpha = m_owner->m_scene->GetPhysicsManager()->GetInterpolationAlpha();
        vec3 smoothPos = m_kinematicController->GetInterpolatedPosition(alpha);
        m_owner->SetWorldPositionInternal(smoothPos + eyeOffset);
    }

    void PlayerControllerComponent::LateUpdate(float _deltaTime)
    {
        Component::LateUpdate(_deltaTime);
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

    void PlayerControllerComponent::SetJumpTrajectory(const vec3& _trajectory)
    {
        m_jumpTrajectory = _trajectory;
    }

    bool PlayerControllerComponent::IsOnGround() const
    {
        if (m_kinematicController)
        {
            return m_kinematicController->IsOnGround();
        }
        return false;
    }

    float PlayerControllerComponent::GetMoveSpeed() const
    {
        return m_moveSpeed;
    }

    float PlayerControllerComponent::GetMouseSensitivity() const
    {
        return m_sensitivity;
    }

    vec3 PlayerControllerComponent::GetJumpTrajectory() const
    {
        return m_jumpTrajectory;
    }

    // PROTECTED -------------------------------------------------------------------------------------------------------

    void PlayerControllerComponent::OnEnable()
    {
        if (m_kinematicController) m_kinematicController->AddToWorld();
    }

    void PlayerControllerComponent::OnDisable()
    {
        if (m_kinematicController) m_kinematicController->RemoveFromWorld();
    }
}
