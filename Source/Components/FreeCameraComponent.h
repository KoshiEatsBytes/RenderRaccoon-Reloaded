
#pragma once
#include "Input/InputManager.h"
#include "Scene/Component.h"
#include "Scene/Components/CameraComponent.h"

namespace RR
{
    class FreeCameraComponent : public CameraComponent
    {
    public:
        COMPONENT(FreeCameraComponent, CameraComponent);

        FreeCameraComponent();
        ~FreeCameraComponent() override;

        void Init() override;
        void PreUpdate(float _deltaTime) override;

        quat GetOrientation() const;
        float GetPitch() const;
        float GetYaw() const;

        void SetPitch(float _pitch);
        void SetYaw(float _yaw);

    private:
        InputManager& m_inputManager;

        quat m_orientation {0.0f, 0.0f, 0.0f, 0.0f};
        float m_sensStep = 0.01f;
        float m_sensMin  = 0.001;
        float m_sensMax  = 1.0;
        float m_sens     = 0.05f;

        float m_moveSpeed   = 8.f;
        float m_sprintSpeed = 14.f;

        float m_pitch = 0.0f;
        float m_yaw   = 0.0f;

        // View clamping
        vec2 m_verticalRotConstraints = {-89.f, 89.0f};
    };
}
