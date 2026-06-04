
#pragma once

#include "Helpers/Concepts.h"

namespace RR
{
    struct TypeInfo;
    class ApplicationData;
    class ApplicationManager;
    class ISubSystem
    {
    public:
        friend class ApplicationManager;
        virtual ~ISubSystem() = default;

    protected:
        ISubSystem();

        virtual bool Init()                                         = 0;
        virtual void Update(float _deltaTime)                       = 0;

        // Non mandatory hooks
        virtual void PreUpdate(float _deltaTime);
        virtual void LateUpdate(float _deltaTime);
        virtual void Destroy();

        // custom RTTI system
        static const TypeInfo& StaticType();
        bool IsA(const TypeInfo& _target) const;
        virtual const TypeInfo& GetType() const;

        template<SubSystemType T>
        bool IsA() const
        {
            return IsA(T::StaticType());
        }

        ApplicationData& m_appData;
    };
}
