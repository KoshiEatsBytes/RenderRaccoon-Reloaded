#pragma once

#include <array>

namespace RR
{
    class InputManager
    {
    private:
        InputManager();
        ~InputManager();

    public:
        // Delete copy & copy assignment
        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;

        // Delete move & move assignment
        InputManager(InputManager&&) = delete;
        InputManager& operator=(InputManager&&) = delete;

        // enforces only engine to instantiate this class
        friend class Engine;

        void SetKeyPressed(const int& key, const bool& pressed);
        bool IsKeyPressed(const int &key) const;

    private:
        // Stores key states (if pressed or released)
        std::array<bool, 384> m_keys = {false};
    };
}

