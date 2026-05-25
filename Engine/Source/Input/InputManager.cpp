
#include "InputManager.h"

#include "Helpers/Printer.hpp"

namespace RR
{
    // PRIVATE ---------------------------------------------------------------------------------------------------------

    InputManager::InputManager() = default;

    InputManager::~InputManager() = default;

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    void InputManager::SetKeyPressed(const int& _key, const bool& _pressed)
    {
        if (_key < 0 || _key >= static_cast<int>(m_keys.size()))
        {
            Warn("[INPUT] Key out of range is being set, discarding.");
            return;
        }

        m_keys[_key] = _pressed;
    }

    bool InputManager::IsKeyPressed(const int& _key) const
    {
        if (_key < 0 || _key >= static_cast<int>(m_keys.size()))
        {
            Warn("[INPUT] Key out of range is being requested, discarding.");
            return false;
        }

        return m_keys[_key];
    }
}
