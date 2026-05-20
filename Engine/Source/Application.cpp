
#include "Application.h"

namespace RR
{
    Application::Application()
    = default;

    Application::~Application()
    = default;

    void Application::SetShouldClose(const bool &val)
    {
        m_shouldClose = val;
    }

    bool Application::ShouldClose() const
    {
        return m_shouldClose;
    }
}
