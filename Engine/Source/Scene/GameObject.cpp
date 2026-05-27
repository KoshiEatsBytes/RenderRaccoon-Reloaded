
#include "GameObject.h"
#include "glm/gtc/matrix_transform.hpp"

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
        // Update each component
        for (const auto& component : m_components)
        {
            component->Update(_deltaTime);
        }

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

    /**
     * @brief Adds a component to the current GameObject
     * @param _component Ptr to the component you want to add
     */
    void GameObject::AddComponent(Component* _component)
    {
        m_components.emplace_back(_component);
        _component->m_owner = this;
    }

    void GameObject::MarkForDestroy()
    {
        m_isAlive = false;
    }

    // GETTER / SETTERS ------------------------------------------------------------------------------------------------

    const std::string& GameObject::GetName() const
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

    const Vec3& GameObject::GetPosition() const
    {
        return m_position;
    }

    void GameObject::SetPosition(const Vec3& _pos)
    {
        m_position = _pos;
    }

    const Vec3& GameObject::GetRotation() const
    {
        return m_rotation;
    }

    void GameObject::SetRotation(const Vec3& _rot)
    {
        m_rotation = _rot;
    }

    const Vec3& GameObject::GetScale() const
    {
        return m_scale;
    }

    void GameObject::SetScale(const Vec3& _scale)
    {
        m_scale = _scale;
    }

    /**
     * @brief Return a matrix combining pos rot and scale in local space
     * @return Local transform matrix
     */
    Mat4 GameObject::GetLocalTransform() const
    {
        Mat4 mat = Mat4(1.0f);

        // Translation
        mat = glm::translate(mat, m_position);

        // Rotation
        mat = glm::rotate(mat, m_rotation.x, Vec3(1.0f, 0.0f, 0.0f)); // X axis rot
        mat = glm::rotate(mat, m_rotation.y, Vec3(0.0f, 1.0f, 0.0f)); // Y axis rot
        mat = glm::rotate(mat, m_rotation.z, Vec3(0.0f, 0.0f, 1.0f)); // Z axis rot

        // Scale
        mat = glm::scale(mat, m_scale);

        return mat;
    }

    /**
     * @brief Return a matrix combining pos rot and scale in world space
     * @return World space matrix
     */
    Mat4 GameObject::GetWorldTransform() const
    {
        // consider parent transform
        if (m_parent)
        {
            // Chain transforms up to root
            return m_parent->GetWorldTransform() * GetLocalTransform();
        }

        return GetLocalTransform();
    }
}
