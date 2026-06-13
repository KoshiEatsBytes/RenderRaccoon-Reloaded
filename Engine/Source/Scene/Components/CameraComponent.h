
#pragma once
#include "Scene/Component.h"
#include "../../Helpers/Types.h"

namespace RR
{
    class CameraComponent : public Component
    {
    public:
        COMPONENT(CameraComponent, Component);

        CameraComponent();
        ~CameraComponent() override;

        void Update(float _deltaTime) override;

        mat4 GetViewMatrix() const;
        mat4 GetProjectionMatrix(float _aspect) const;

        void SetParameters(float _fov, float _nearPlane, float _farPlane);
        void SetFOV(float _fov);
        void SetNearPlane(float _nearPlane);
        void SetFarPlane(float _farPlane);

        float GetFov() const;
        float GetNearPlane() const;
        float GetFarPlane() const;

    private:
        float m_fov = 60.0f;
        float m_nearPlane = 0.1f;
        float m_farPlane = 1000.0f;
    };
}
