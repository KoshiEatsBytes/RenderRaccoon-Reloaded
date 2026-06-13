
#pragma once
#include "../Helpers/Types.h"
#include "../Helpers/TypeInfo.h"

namespace RR
{
    class GameObject;

    class Component
    {
    public:
        friend class GameObject;

        Component();
        virtual ~Component();

        virtual void Init();
        virtual void PreUpdate(float _deltaTime);
        virtual void Update(float _deltaTime)       = 0;
        virtual void LateUpdate(float _deltaTime);

        virtual int GetExecutionOrder() const;
        GameObject* GetOwner() const;

        void SetEnabled(bool _enabled);
        bool IsEnabled() const;

    protected:
        virtual void OnEnable();
        virtual void OnDisable();

        // Custom RTTI — mirrors GameObject 
        static const TypeInfo& StaticType();
        bool IsA(const TypeInfo& _target) const;
        virtual const TypeInfo& GetType() const;

        bool m_enabled = true;
        // for handling the owner
        GameObject* m_owner = nullptr;
    };
}
