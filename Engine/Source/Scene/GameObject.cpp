
#include "GameObject.h"

namespace RR
{
    // PROTECTED -------------------------------------------------------------------------------------------------------

    GameObject::GameObject()
    = default;

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    GameObject::~GameObject()
    = default;

    void GameObject::Update(const float _deltaTime)
    {
        // Update each child, or destroy marked
        for (auto it = m_children.begin(); it != m_children.end();)
        {
            if ((*it)->IsAlive())
            {
                (*it)->Update(_deltaTime);
                ++it;
            }
            else
            {
                // Erase it from list, ptr gets freed when out of scope.
                it = m_children.erase(it);
            }
        }
    }

    void GameObject::MarkForDestroy()
    {
        m_isAlive = false;
    }

    // GETTER / SETTERS ------------------------------------------------------------------------------------------------

    const std::string & GameObject::GetName() const
    {
        return m_name;
    }

    void GameObject::SetName(const std::string& _name)
    {
        m_name = _name;
    }

    GameObject* GameObject::GetParent()
    {
        return m_parent;
    }

    bool GameObject::IsAlive() const
    {
        return m_isAlive;
    }
}
