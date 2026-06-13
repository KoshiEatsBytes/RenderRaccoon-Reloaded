
#include "InputManager.h"

#include "Helpers/Printer.hpp"

namespace RR
{
    // PRIVATE ---------------------------------------------------------------------------------------------------------

    InputManager::InputManager() = default;

    InputManager::~InputManager() = default;

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    void InputManager::SetUICapture(bool _keyboard, bool _mouse)
    {
        m_uiCaptureKeyboard = _keyboard;
        m_uiCaptureMouse    = _mouse;
    }

    // KEYBOARD --------------------------------------------------------------------------------------------------------

    void InputManager::SetKeyPressed(const int _key, const bool _pressed)
    {
        if (_key < 0 || _key >= static_cast<int>(m_keys.size()))
        {
            Warn("[INPUT - KEYBOARD] Key out of range is being set, discarding.");
            return;
        }

        m_keys[_key] = _pressed;
    }

    bool InputManager::IsKeyPressed(const int _key) const
    {
        if (m_uiCaptureKeyboard) return false;

        if (_key < 0 || _key >= static_cast<int>(m_keys.size()))
        {
            Warn("[INPUT - KEYBOARD] Key out of range is being requested, discarding.");
            return false;
        }

        return m_keys[_key];
    }

    // MOUSE -----------------------------------------------------------------------------------------------------------

    void InputManager::SetMouseButtonPressed(int _button, bool _pressed)
    {
        if (_button < 0 || _button >= static_cast<int>(m_mouseKeys.size()))
        {
            Warn("[INPUT - MOUSE] Key out of range is being set, discarding.");
            return;
        }

        m_mouseKeys[_button] = _pressed;
    }

    bool InputManager::IsMouseButtonPressed(int _button) const
    {
        if (m_uiCaptureMouse) return false;

        if (_button < 0 || _button >= static_cast<int>(m_mouseKeys.size()))
        {
            Warn("[INPUT - MOUSE] Key out of range is being set, discarding.");
            return false;
        }

        return m_mouseKeys[_button];
    }

    // GET/SETS --------------------------------------------------------------------------------------------------------

    void InputManager::SetMousePositionOld(const vec2& _pos)
    {
        m_mousePosOld = _pos;
    }

    const vec2& InputManager::GetMousePositionOld() const
    {
        return m_mousePosOld;
    }

    void InputManager::SetMousePositionCurrent(const vec2& _pos)
    {
        m_mousePosCurrent = _pos;
    }

    const vec2& InputManager::GetMousePositionCurrent() const
    {
        return m_mousePosCurrent;
    }

    void InputManager::SetMousePositionChanged(bool _changed)
    {
        m_mousePosChanged = _changed;
    }

    bool InputManager::GetMousePositionChanged() const
    {
        if (m_uiCaptureMouse) return false;

        return m_mousePosChanged;
    }

    void InputManager::SetScrollDelta(float _delta)
    {
        m_scrollDelta += _delta;
    }

    float InputManager::GetScrollDelta() const
    {
        return m_scrollDelta;
    }

    void InputManager::ResetScrollDelta()
    {
        m_scrollDelta = 0;
    }
}
