
#include "Component.h"

namespace RR
{

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    Component::Component()
    = default;

    Component::~Component()
    = default;

    void Component::Init()
    {
    }

    int Component::GetExecutionOrder() const
    {
        return 0;
    }

    GameObject* Component::GetOwner() const
    {
        return m_owner;
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    // Init component ID
    sizeT Component::nextID = 1;
}
