
#pragma once
#include "Scene/Component.h"
#include "../../Helpers/Types.h"

namespace RR
{
    class CameraComponent : public Component
    {
    public:
        COMPONENT(CameraComponent);

        CameraComponent();
        ~CameraComponent() override;

        void Update(float _deltaTime) override;

        mat4 GetViewMatrix() const;
        mat4 GetProjectionMatrix(float _aspect) const;

    private:
        float m_fov = 60.0f;
        float m_nearPlane = 0.1f;
        float m_farPlane = 1000.0f;
    };
}
