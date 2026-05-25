
#pragma once

namespace RR
{
    /**
     * @brief Interface class to apply to program
     * to allow compatibility with RenderRaccoon
     */
    class Application
    {
    public:
        Application();
        virtual ~Application();

        // Derived class must implement
        virtual bool Init()                             = 0;
        virtual void Update(float _deltaTime)           = 0;
        virtual void Destroy()                          = 0;

        void SetShouldClose(const bool& val);
        bool GetShouldClose() const;

    private:
        bool m_shouldClose = false;
    };
}


