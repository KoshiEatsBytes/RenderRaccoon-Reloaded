
#include "GameObject.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Engine.h"
#include "Helpers/Printer.hpp"
#include "Helpers/GLTFLib.hpp"
#include "Physics/RigidBody.h"
#include "Scene/Components/PhysicsComponent.h"

namespace RR
{
    // PROTECTED -------------------------------------------------------------------------------------------------------

    GameObject::GameObject()
    = default;


    // PUBLIC ----------------------------------------------------------------------------------------------------------

    GameObject::~GameObject()
    = default;

    void GameObject::Init()
    {
    }

    void GameObject::PreUpdate(const float _deltaTime)
    {
        if (!m_active) return;
        if (!m_ticked) m_ticked = true;

        // Run the pre-update of each component
        for (const auto& component : m_components)
        {
            if (component->IsEnabled()) component->PreUpdate(_deltaTime);
        }

        for (const auto& child: m_children)
        {
            child->PreUpdate(_deltaTime);
        }
    }

    void GameObject::Update(const float _deltaTime)
    {
        if (!m_active) return;

        // Update each component
        for (const auto& component : m_components)
        {
            if (component->IsEnabled()) component->Update(_deltaTime);
        }

        for (const auto& child: m_children)
        {
            child->Update(_deltaTime);
        }
    }

    void GameObject::LateUpdate(const float _deltaTime)
    {
        if (!m_active) return;

        // run late-update on each component
        for (const auto& component : m_components)
        {
            if (component->IsEnabled()) component->LateUpdate(_deltaTime);
        }

        for (const auto& child: m_children)
        {
            child->LateUpdate(_deltaTime);
        }
    }

    /**
     * @brief Adds a component to the current GameObject
     * @param _component Ptr to the component you want to add
     */
    void GameObject::AddComponent(Component* _component)
    {
        if (m_ticked)
        {
            Warn("[GAME-OBJECT - ADD COMPONENT] Tried adding component at RunTime to GO: '", m_name, "' Discarding.");
            delete _component;
            return;
        }

        if (!_component)
        {
            Warn("[GAME-OBJECT - ADD COMPONENT] Tried adding INVALID component to GO: '", m_name, "' Discarding.");
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
        if (m_alive)
        {
            m_alive = false;
            m_scene->EnqueueDestroy(this);
        }
    }

    GameObject* GameObject::LoadGLTF(const std::string& _path) const
    {
        return CGLTFLib::LoadGLTF(_path, m_scene);
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

        const auto ownership = GetPhysicsOwnership();
        if (ownership == PhysicsOwnership::STATIC || ownership == PhysicsOwnership::INHERITED)
        {
            Error("[GAME-OBJECT SET PARENT] Parenting discarded for '", m_name,
                  "' — GO is a STATIC body or part of a physics body's hierarchy.");
            return false;
        }

        return m_scene->SetParent(this, _parent);
    }

    GameObject* GameObject::GetParent() const
    {
        return m_parent;
    }

    GameObject* GameObject::FindObjectByName(const std::string& _name, bool _searchDescendants)
    {
        if (m_name == _name)
        {
            return this;
        }

        if (!_searchDescendants)
        {
            for (auto& child : m_children)
            {
                if (child->GetName() == _name)
                {
                    return child.get();
                }
            }

            return nullptr;
        }

        for (auto& child : m_children)
        {
            if (auto res = child->FindObjectByName(_name, true))
            {
                return res;
            }
        }

        return nullptr;
    }

    std::vector<GameObject*> GameObject::FindObjectsByName(const std::string& _name, bool _searchDescendants)
    {
        std::vector<GameObject*> result;

        if (_searchDescendants)
        {
            FindObjectsByName(_name, result);
        }
        else
        {
            if (m_name == _name)
            {
                result.push_back(this);
            }

            for (auto& child : m_children)
            {
                if (child->GetName() == _name)
                {
                    result.push_back(child.get());
                }
            }
        }

        return result;
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
        switch (GetPhysicsOwnership())
        {
            case PhysicsOwnership::DYNAMIC:
                Warn("[GAME-OBJECT - POS] Discarded SetPosition on '", m_name,
                     "' — DYNAMIC body owns its transform. Use PhysicsComponent::Teleport().");
                return;

            case PhysicsOwnership::STATIC:
                Warn("[GAME-OBJECT - POS] Discarded SetPosition on '", m_name,
                     "' — STATIC body cannot move. Use KINEMATIC or PhysicsComponent::Rebuild().");
                return;

            case PhysicsOwnership::CHARACTER:
                Warn("[GAME-OBJECT - POS] Discarded SetPosition on '", m_name,
                     "' — controlled by a character controller. ",
                     "Use PlayerControllerComponent::Teleport() instead.");
                return;

            case PhysicsOwnership::INHERITED:
                Warn("[GAME-OBJECT - POS] Discarded SetPosition on '", m_name,
                     "' — part of a physics body's hierarchy. "
                     "Compound layout is baked at Init. Use PhysicsComponent::Rebuild() to refresh.");
                return;

            default:
                break;
        }

        m_position = _pos;
    }

    void GameObject::SetWorldPosition(const vec3& _pos)
    {
        switch (GetPhysicsOwnership())
        {
            case PhysicsOwnership::DYNAMIC:
                Warn("[GAME-OBJECT - POS] Discarded SetWorldPosition on '", m_name,
                     "' — DYNAMIC body owns its transform. Use PhysicsComponent::Teleport().");
                return;

            case PhysicsOwnership::STATIC:
                Warn("[GAME-OBJECT - POS] Discarded SetWorldPosition on '", m_name,
                     "' — STATIC body cannot move. Use KINEMATIC or PhysicsComponent::Rebuild().");
                return;

            case PhysicsOwnership::CHARACTER:
                Warn("[GAME-OBJECT - POS] Discarded SetWorldPosition on '", m_name,
                     "' — controlled by a character controller. ",
                     "Use PlayerControllerComponent::Teleport() instead.");
                return;

            case PhysicsOwnership::INHERITED:
                Warn("[GAME-OBJECT - POS] Discarded SetWorldPosition on '", m_name,
                     "' — part of a physics body's hierarchy. "
                     "Compound layout is baked at Init. Use PhysicsComponent::Rebuild() to refresh.");
                return;

            default:
                break;
        }

        SetWorldPositionInternal(_pos);
    }

    quat GameObject::GetRotation() const
    {
        return m_rotation;
    }

    void GameObject::SetRotation(const quat& _rot)
    {
        switch (GetPhysicsOwnership())
        {
            case PhysicsOwnership::DYNAMIC:
                Warn("[GAME-OBJECT - ROT] Discarded SetRotation on '", m_name,
                     "' — DYNAMIC body owns its transform. Use PhysicsComponent::SetRotation().");
                return;

            case PhysicsOwnership::STATIC:
                Warn("[GAME-OBJECT - ROT] Discarded SetRotation on '", m_name,
                     "' — STATIC body cannot move. Use KINEMATIC or PhysicsComponent::Rebuild().");
                return;

            case PhysicsOwnership::CHARACTER:
                Warn("[GAME-OBJECT - ROT] Discarded SetRotation on '", m_name,
                     "' — controlled by a character controller. ",
                     "Use PlayerControllerComponent::SetLookRotation() instead.");
                return;

            case PhysicsOwnership::INHERITED:
                Warn("[GAME-OBJECT - ROT] Discarded SetRotation on '", m_name,
                     "' — part of a physics body's hierarchy. "
                     "Compound layout is baked at Init. Use PhysicsComponent::Rebuild() to refresh.");
                return;

            default:
                break;
        }

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
        switch (GetPhysicsOwnership())
        {
            case PhysicsOwnership::DYNAMIC:
                Warn("[GAME-OBJECT - ROT] Discarded SetWorldRotation on '", m_name,
                     "' — DYNAMIC body owns its transform. Use PhysicsComponent::SetRotation().");
                return;

            case PhysicsOwnership::STATIC:
                Warn("[GAME-OBJECT - ROT] Discarded SetWorldRotation on '", m_name,
                     "' — STATIC body cannot move. Use KINEMATIC or PhysicsComponent::Rebuild().");
                return;

            case PhysicsOwnership::CHARACTER:
                Warn("[GAME-OBJECT - ROT] Discarded SetWorldRotation on '", m_name,
                     "' — controlled by a character controller. ",
                     "Use PlayerControllerComponent::SetLookRotation() instead.");
                return;

            case PhysicsOwnership::INHERITED:
                Warn("[GAME-OBJECT - ROT] Discarded SetWorldRotation on '", m_name,
                     "' — part of a physics body's hierarchy. "
                     "Compound layout is baked at Init. Use PhysicsComponent::Rebuild() to refresh.");
                return;

            default:
                break;
        }

        SetWorldRotationInternal(_rot);
    }

    const vec3& GameObject::GetScale() const
    {
        return m_scale;
    }

    void GameObject::SetScale(const vec3& _scale)
    {
        switch (GetPhysicsOwnership())
        {
            case PhysicsOwnership::DYNAMIC:
            case PhysicsOwnership::KINEMATIC:
            case PhysicsOwnership::STATIC:
                Warn("[GAME-OBJECT - SCALE] Discarded SetScale on '", m_name,
                     "' — Object has Rigidbody. Use PhysicsComponent::SetScale() (EXPENSIVE — rebuilds the body).");
                return;

            case PhysicsOwnership::CHARACTER:
                Warn("[GAME-OBJECT - SCALE] Discarded SetScale on '", m_name,
                     "' — character controllers can't be scaled. Recreate the controller with a different capsule size.");
                return;

            case PhysicsOwnership::INHERITED:
                Warn("[GAME-OBJECT - SCALE] Discarded SetScale on '", m_name,
                     "' — part of a physics body's hierarchy. "
                     "Compound layout is baked at Init. Use PhysicsComponent::Rebuild() to refresh.");
                return;

            case PhysicsOwnership::NONE:
                break;
        }

        m_scale = _scale;
    }

    vec3 GameObject::GetWorldLossyScale() const
    {
        // Generates Lossy Scale, prevents heavy shearing
        vec3 scale, translation, skew;
        quat rotation;
        vec4 perspective;
        glm::decompose(GetWorldTransform(), scale, rotation, translation, skew, perspective);
        return scale;
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

    // PROTECTED -------------------------------------------------------------------------------------------------------

    const TypeInfo& GameObject::StaticType()
    {
        static constexpr TypeInfo info {"GameObject", nullptr};
        return info;
    }

    bool GameObject::IsA(const TypeInfo &_target) const
    {
        for (const TypeInfo* t = &GetType(); t != nullptr; t = t->base)
        {
            if (t == &_target) return true;
        }
        return false;
    }

    const TypeInfo& GameObject::GetType() const
    {
        return StaticType();
    }

    PhysicsOwnership GameObject::GetPhysicsOwnership() const
    {
        if (m_physicsOwnership != PhysicsOwnership::NONE)
        {
            return m_physicsOwnership;
        }
        if (m_parentPhysicsOwnership != PhysicsOwnership::NONE)
        {
            return PhysicsOwnership::INHERITED;
        }

        return PhysicsOwnership::NONE;
    }

    void GameObject::SetWorldPositionInternal(const vec3& _pos)
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

    void GameObject::SetWorldRotationInternal(const quat &_rot)
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

    void GameObject::SetScaleInternal(const vec3 &_scale)
    {
        m_scale = _scale;
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    // Made private to prevent involuntary injection
    void GameObject::FindObjectsByName(const std::string &_name, std::vector<GameObject*>& _result)
    {
        if (m_name == _name)
        {
            _result.push_back(this);
        }

        for (auto& child : m_children)
        {
            child->FindObjectsByName(_name, _result);
        }
    }
}
