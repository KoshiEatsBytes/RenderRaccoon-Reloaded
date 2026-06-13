
#pragma once
#include <memory>

#include "scene/Component.h"
#include "Physics/KinematicCharacterController.h"

namespace RR
{
    class PlayerControllerComponent : public Component
    {
    public:
        COMPONENT(PlayerControllerComponent, Component)

        PlayerControllerComponent();
        PlayerControllerComponent(float _capsuleHeight, float _capsuleRadius);
        ~PlayerControllerComponent() override;

        void Init() override;
        void PreUpdate(float _deltaTime) override;
        void Update(float _deltaTime) override;
        void LateUpdate(float _deltaTime) override;

        void Teleport(const vec3& _worldPos);
        void SetLookRotation(float _yawDegrees, float _pitchDegrees);
        vec2 GetLookRotation() const;

        void SetMoveSpeed(float _speed);
        void SetMouseSensitivity(float _sens);
        void SetJumpTrajectory(const vec3& _trajectory);

        bool IsOnGround() const;
        bool IsMidJump() const;
        float GetMoveSpeed() const;
        float GetMouseSensitivity() const;
        vec3 GetJumpTrajectory() const;

    protected:
        void OnEnable() override;
        void OnDisable() override;

    private:
        std::unique_ptr<KinematicCharacterController> m_kinematicController;

        float m_sensitivity = 0.05f;
        float m_moveSpeed = 6.f;

        bool m_midJump = false;
        vec3 m_jumpTrajectory {0.0f, 5.0f, 0.0f};

        float m_capsuleHeight = 1.2f;
        float m_capsuleRadius = 0.4f;

        float m_xRot = 0.0f;
        float m_yRot = 0.0f;

        // View clamping
        vec2 m_verticalRotConstraints = {-89.f, 89.0f};
    };
}

