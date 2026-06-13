
#pragma once
#include "Scene/Component.h"
#include "../../Helpers/Types.h"

namespace RR
{
    class LightComponent : public Component
    {
    public:
        COMPONENT(LightComponent, Component);

        LightComponent();
        ~LightComponent() override;

        void Init() override;
        void Update(float _deltaTime) override;

        const vec3& GetColor() const;
        void SetColor(const vec3& _color);

    private:
        vec3 m_color = vec3(1.0f); // default white
    };
}
