
#pragma once

namespace RR
{
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
        virtual void PreUpdate(float _deltaTime)                    = 0;
        virtual void Update(float _deltaTime)                       = 0;
        virtual void LateUpdate(float _deltaTime)                   = 0;
        virtual void Destroy()                                      = 0;

        ApplicationData& m_appData;
    };
}
