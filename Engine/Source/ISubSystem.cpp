
#include "ISubSystem.h"
#include "Engine.h"

namespace RR
{
    ISubSystem::ISubSystem() : m_appData(Engine::GetInstance().GetAppData())
    {
    }

    void ISubSystem::PreUpdate(float _deltaTime)
    {
    }

    void ISubSystem::LateUpdate(float _deltaTime)
    {
    }

    void ISubSystem::Destroy()
    {
    }

    const TypeInfo& ISubSystem::StaticType()
    {
        static constexpr TypeInfo info {"SubSystem", nullptr};
        return info;
    }

    bool ISubSystem::IsA(const TypeInfo &_target) const
    {
        for (const TypeInfo* t = &GetType(); t != nullptr; t = t->base)
        {
            if (t == &_target) return true;
        }
        return false;
    }

    const TypeInfo& ISubSystem::GetType() const
    {
        return StaticType();
    }
}
