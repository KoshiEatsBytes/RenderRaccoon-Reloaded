#pragma once
#include <array>

#include "Types.h"

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

        void SetKeyPressed(int _key, bool _pressed);
        bool IsKeyPressed(int _key) const;
        void SetMouseButtonPressed(int _button, bool _pressed);
        bool IsMouseButtonPressed(int _button) const;

        // Set/Gets
        void SetMousePositionOld(const Vec2& _pos);
        const Vec2& GetMousePositionOld() const;
        void SetMousePositionCurrent(const Vec2& _pos);
        const Vec2& GetMousePositionCurrent() const;

    private:
        std::array<bool, 384> m_keys = {false};
        std::array<bool, 16> m_mouseKeys = {false};

        Vec2 m_mousePosOld = Vec2(0.0f);
        Vec2 m_mousePosCurrent = Vec2(0.0f);


    };
}

