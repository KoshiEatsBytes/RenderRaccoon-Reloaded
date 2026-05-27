
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
    = default;

    void PlayerControllerComponent::Update(float _deltaTime)
    {
        auto& inputManager = Engine::GetInstance().GetInputManager();
        auto rotation = m_owner->GetRotation();

        // rotate around Y axis for horizontal movement and around X for vertical
        if (inputManager.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
        {
            const auto& oldPos = inputManager.GetMousePositionOld();
            const auto& currPos = inputManager.GetMousePositionCurrent();

            float deltaX = currPos.x - oldPos.x;
            float deltaY = currPos.y - oldPos.y;

            // rot around Y axis
            rotation.y -= deltaX * m_sensitivity * _deltaTime;
            // rot around Y axis
            rotation.x -= deltaY * m_sensitivity * _deltaTime;

            // set rot to owner GO
            m_owner->SetRotation(rotation);
        }

        // Movement relative to camera rot
        Mat4 rotMat(1.0f);
        rotMat = glm::rotate(rotMat, rotation.x, Vec3(1.0f, 0.0f, 0.0f));
        rotMat = glm::rotate(rotMat, rotation.y, Vec3(0.0f, 1.0f, 0.0f));
        rotMat = glm::rotate(rotMat, rotation.z, Vec3(0.0f, 0.0f, 1.0f));

        // forward along z axis, right x
        Vec3 forward = glm::normalize(Vec3(rotMat * Vec4(0.0f, 0.0f, -1.0f, 0.0f)));
        Vec3 right   = glm::normalize(Vec3(rotMat * Vec4(1.0f, 0.0f, 0.0f, 0.0f)));

        auto position = m_owner->GetPosition();

        // Left/Right
        if (inputManager.IsKeyPressed(GLFW_KEY_A))
        {
            position += right * m_moveSpeed * _deltaTime;
        }
        if (inputManager.IsKeyPressed(GLFW_KEY_D))
        {
            position -= right * m_moveSpeed * _deltaTime;
        }

        // Up/Down
        if (inputManager.IsKeyPressed(GLFW_KEY_W))
        {
            position += forward * m_moveSpeed * _deltaTime;
        }
        if (inputManager.IsKeyPressed(GLFW_KEY_S))
        {
            position -= forward * m_moveSpeed * _deltaTime;
        }

        m_owner->SetPosition(position);
    }
}
