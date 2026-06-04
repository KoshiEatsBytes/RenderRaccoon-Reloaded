
#include "ISubSystem.h"
#include "Engine.h"

namespace RR
{
    ISubSystem::ISubSystem() : m_appData(Engine::GetInstance().GetAppData())
    {
    }
}
