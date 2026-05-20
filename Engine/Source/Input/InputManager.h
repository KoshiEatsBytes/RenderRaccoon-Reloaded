#pragma once

#include <array>

namespace RR
{
    class InputManager
    {
    private:
        InputManager();
        ~InputManager();

        // Delete copy & copy assignment
        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;

        // Delete move & move assignment
        InputManager(InputManager&&) = delete;
        InputManager& operator=(InputManager&&) = delete;

        // Allow access only to engine
        friend class Engine;

    public:
        void SetKeyPressed(const int& key, const bool& pressed);
        bool IsKeyPressed(const int &key) const;

    private:
        // Stores key states (if pressed or released)
        std::array<bool, 256> m_keys = {false};
    };
}

