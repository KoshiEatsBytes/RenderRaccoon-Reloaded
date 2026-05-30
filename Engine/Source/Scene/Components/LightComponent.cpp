
#include "LightComponent.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    LightComponent::LightComponent()
    = default;

    LightComponent::~LightComponent()
    = default;

    void LightComponent::Update(float _deltaTime)
    {
    }

    const vec3& LightComponent::GetColor() const
    {
        return m_color;
    }

    void LightComponent::SetColor(const vec3& _color)
    {
        m_color = _color;
    }
}
