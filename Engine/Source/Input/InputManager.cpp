
#include "InputManager.h"

#include "Helpers/Printer.hpp"

namespace RR
{
    // PRIVATE ---------------------------------------------------------------------------------------------------------

    InputManager::InputManager() = default;

    InputManager::~InputManager() = default;

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    void InputManager::SetKeyPressed(const int& key, const bool& pressed)
    {
        if (key < 0 || key >= static_cast<int>(m_keys.size()))
        {
            Warn("[INPUT] Key out of range is being set, discarding.");
            return;
        }

        m_keys[key] = pressed;
    }

    bool InputManager::IsKeyPressed(const int &key) const
    {
        if (key < 0 || key >= static_cast<int>(m_keys.size()))
        {
            Warn("[INPUT] Key out of range is being requested, discarding.");
            return false;
        }

        return m_keys[key];
    }
}
