
#include "Application.h"

namespace rr
{
    Application::Application()
    = default;

    Application::~Application()
    = default;

    void Application::SetShouldClose(const bool &val)
    {
        _shouldClose = val;
    }

    bool Application::ShouldClose() const
    {
        return _shouldClose;
    }
}
