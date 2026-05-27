
#pragma once
#include "Scene/Component.h"
#include "Types.h"

namespace RR
{
    class CameraComponent : public Component
    {
        COMPONENT(CameraComponent);

    public:
        CameraComponent();
        ~CameraComponent() override;

        void Update(float _deltaTime) override;

        Mat4 GetViewMatrix() const;
        Mat4 GetProjectionMatrix(float _aspect) const;

    private:
        float m_fov = 60.0f;
        float m_nearPlane = 0.1f;
        float m_farPlane = 1000.0f;
    };
}
