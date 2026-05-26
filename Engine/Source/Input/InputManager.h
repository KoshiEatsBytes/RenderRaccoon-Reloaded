#pragma once

#include <array>

namespace RR
{
    class InputManager
    {
        InputManager();
        ~InputManager();

    public:
        // Only engine can construct this class
        friend class Engine;

        // Delete copy
        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;

        // Delete move
        InputManager(InputManager&&) noexcept = delete;
        InputManager& operator=(InputManager&&) noexcept = delete;

        // METHODS -----------------------------------------------------------------------------------------------------

        void SetKeyPressed(const int& _key, const bool& _pressed);
        bool IsKeyPressed(const int& _key) const;

    private:
        std::array<bool, 384> m_keys = {false};
    };
}

