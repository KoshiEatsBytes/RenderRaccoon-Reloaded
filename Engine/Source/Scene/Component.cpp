
#include "Component.h"

namespace RR
{

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    Component::Component()
    = default;

    Component::~Component()
    = default;

    GameObject* Component::GetOwner() const
    {
        return m_owner;
    }
}
