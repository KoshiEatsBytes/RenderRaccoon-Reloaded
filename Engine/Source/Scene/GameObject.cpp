
#include "GameObject.h"

#include "Engine.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Helpers/Printer.hpp"
#include "Helpers/GLTFLib.hpp"

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
        if (!m_active) return;

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
        if (!_component)
        {
            Warn("[GAME-OBJECT - ADD COMPONENT] Tried adding INVALID component to GO: '", m_name, "'");
            return;
        }

        m_components.emplace_back(_component);
        _component->m_owner = this;

        // Sorts components by execution order
        std::stable_sort(m_components.begin(), m_components.end(),
        [](const auto& a, const auto& b) {
            return a->GetExecutionOrder() < b->GetExecutionOrder();
        });

        _component->Init();
    }

    void GameObject::MarkForDestroy()
    {
        m_alive = false;
    }

    GameObject* GameObject::LoadGLTF(const std::string& _path)
    {
        return CGLTFLib::LoadGLTF(_path);
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

    bool GameObject::SetParent(GameObject* _parent)
    {
        if (!m_scene) return false;

        return m_scene->SetParent(this, _parent);
    }

    GameObject* GameObject::GetParent() const
    {
        return m_parent;
    }

    GameObject* GameObject::GetChildByName(const std::string& _name)
    {
        if (m_name == _name)
        {
            return this;
        }

        // not the most efficent, recursively iterates trough children of children to find
        // GO with specified name
        for (auto& child : m_children)
        {
            if (auto res = child->GetChildByName(_name))
            {
                return res;
            }
        }

        return nullptr;
    }

    const std::vector<std::unique_ptr<GameObject>>& GameObject::GetChildren() const
    {
        return m_children;
    }

    Scene* GameObject::GetScene() const
    {
        return m_scene;
    }

    void GameObject::SetAlive(bool _alive)
    {
        m_alive = _alive;
    }

    bool GameObject::IsAlive() const
    {
        return m_alive;
    }

    void GameObject::SetActive(const bool _active)
    {
        m_active = _active;
    }

    bool GameObject::IsActive() const
    {
        return m_active;
    }

    vec3 GameObject::GetPosition() const
    {
        return m_position;
    }

    vec3 GameObject::GetWorldPosition() const
    {
        const vec4 hom = GetWorldTransform() * vec4(0.0f, 0.0f, 0.0f, 1.0f);
        return vec3(hom) / hom.w;
    }

    void GameObject::SetPosition(const vec3& _pos)
    {
        m_position = _pos;
    }

    void GameObject::SetWorldPosition(const vec3& _pos)
    {
        if (m_parent)
        {
            const mat4 inverseParent = glm::inverse(m_parent->GetWorldTransform());
            m_position = vec3(inverseParent * vec4(_pos, 1.0f));
        }
        else
        {
            m_position = _pos;
        }
    }

    quat GameObject::GetRotation() const
    {
        return m_rotation;
    }

    void GameObject::SetRotation(const quat& _rot)
    {
        m_rotation = _rot;
    }

    quat GameObject::GetWorldRotation() const
    {
        if (m_parent)
        {
            return m_parent->GetWorldRotation() * m_rotation;
        }

        return m_rotation;
    }

    void GameObject::SetWorldRotation(const quat& _rot)
    {
        if (m_parent)
        {
            quat inverseParent = glm::inverse(m_parent->GetWorldRotation());
            m_rotation = inverseParent * _rot;
        }
        else
        {
            m_rotation = _rot;
        }
    }

    const vec3& GameObject::GetScale() const
    {
        return m_scale;
    }

    void GameObject::SetScale(const vec3& _scale)
    {
        m_scale = _scale;
    }

    vec3 GameObject::GetWorldScale() const
    {
        if (m_parent)
        {
            return m_parent->GetWorldScale() * m_scale;
        }

        return m_scale;
    }

    /**
     * @brief Return a matrix combining pos rot and scale in local space
     * @return Local transform matrix
     */
    mat4 GameObject::GetLocalTransform() const
    {
        mat4 mat = mat4(1.0f);

        // Translation
        mat = glm::translate(mat, m_position);

        // Rotation
        mat = mat * glm::mat4_cast(m_rotation);

        // Scale
        mat = glm::scale(mat, m_scale);

        return mat;
    }

    /**
     * @brief Return a matrix combining pos rot and scale in world space
     * @return World space matrix
     */
    mat4 GameObject::GetWorldTransform() const
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
