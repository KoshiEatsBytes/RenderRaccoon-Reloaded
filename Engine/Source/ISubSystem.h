
#pragma once
#include "Engine.h"
#include "Helpers/ApplicationData.h"

namespace RR
{
    class SceneManager;
    class ISubSystem
    {
    protected:
        friend class SceneManager;

        ISubSystem() : m_appData(Engine::GetInstance().GetAppData()) {}
        virtual ~ISubSystem()                                       = 0;

        virtual bool Init()                                         = 0;
        virtual void PreUpdate(float _deltaTime)                    = 0;
        virtual void Update(float _deltaTime)                       = 0;
        virtual void LateUpdate(float _deltaTime)                   = 0;
        virtual void Destroy()                                      = 0;

        ApplicationData& m_appData;
    };
}
