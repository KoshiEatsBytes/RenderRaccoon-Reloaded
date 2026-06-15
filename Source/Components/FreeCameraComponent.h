
#pragma once
#include "Input/InputManager.h"
#include "Scene/Component.h"
#include "Scene/Components/CameraComponent.h"

namespace RR
{
    // Holds movement for a camera frame
    struct CameraIntent
    {
        vec3  moveAxis {0.0f};
        bool  sprint     = false;
        float lookDeltaX = 0.0f;
        float lookDeltaY = 0.0f;
        float sensScroll = 0.0f;
    };

    class FreeCameraComponent : public CameraComponent
    {
    public:
        COMPONENT(FreeCameraComponent, CameraComponent);

        FreeCameraComponent();
        ~FreeCameraComponent() override;

        void Init() override;
        void PreUpdate(float _deltaTime) override;

        void ApplyIntent(const CameraIntent &_in, float _deltaTime);
        CameraIntent GatherLiveIntent() const;

        quat GetOrientation() const;
        float GetPitch() const;
        float GetYaw() const;

        void SetPitch(float _pitch);
        void SetYaw(float _yaw);

        void SetSpeed(float _speed);
        void SetSprintSpeed(float _speed);

    private:
        InputManager& m_inputManager;

        quat m_orientation {1.0f, 0.0f, 0.0f, 0.0f};
        float m_sensStep = 0.002f;
        float m_sensMin  = 0.001f;
        float m_sensMax  = 1.0f;
        float m_sens     = 0.05f;

        float m_moveSpeed   = 8.f;
        float m_sprintSpeed = 14.f;

        float m_pitch = 0.0f;
        float m_yaw   = 0.0f;

        // View clamping
        vec2 m_verticalRotConstraints = {-89.f, 89.0f};
    };
}
