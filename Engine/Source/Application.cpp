
#include "Application.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    Application::Application()
    = default;

    Application::~Application()
    = default;

    // GETTER / SETTERS ------------------------------------------------------------------------------------------------

    void Application::SetShouldClose(const bool &val)
    {
        m_shouldClose = val;
    }

    bool Application::GetShouldClose() const
    {
        return m_shouldClose;
    }
}
