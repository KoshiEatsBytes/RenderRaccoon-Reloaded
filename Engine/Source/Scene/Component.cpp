
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

    void Component::PreUpdate(float _deltaTime)
    {
    }

    void Component::LateUpdate(float _deltaTime)
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

    void Component::SetEnabled(bool _enabled)
    {
        if (m_enabled == _enabled) return;

        m_enabled = _enabled;

        if (_enabled)
        {
            OnEnable();
        }
        else
        {
            OnDisable();
        }
    }

    bool Component::IsEnabled() const
    {
        return m_enabled;
    }

    void Component::OnEnable()
    {
    }

    void Component::OnDisable()
    {
    }

    // RTTI ------------------------------------------------------------------------------------------------------------

    const TypeInfo& Component::StaticType()
    {
        static constexpr TypeInfo info {"Component", nullptr};
        return info;
    }

    bool Component::IsA(const TypeInfo& _target) const
    {
        for (const TypeInfo* t = &GetType(); t != nullptr; t = t->base)
        {
            if (t == &_target) return true;
        }
        return false;
    }

    const TypeInfo& Component::GetType() const
    {
        return StaticType();
    }
}
