
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
        Mat4 GetProjectionMatrix() const;

    private:
        

    };
}
