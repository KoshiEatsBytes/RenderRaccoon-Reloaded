
#pragma once

namespace rr
{
    class Application
    {
    public:
        Application();
        ~Application();

        virtual bool Init()                         = 0;
        virtual void Update(const float& deltaTime) = 0;
        virtual void Destroy()                      = 0;

        void SetShouldClose(const bool& val);
        bool ShouldClose() const;

    private:
        bool _shouldClose = false;
    };
}


