
#pragma once
#include <string>
#include <vector>
#include <memory>

#include "Helpers/Concepts.h"
#include "Helpers/Types.h"
#include "Component.h"

namespace RR
{
    enum class PhysicsOwnership : uint8_t
    {
        NONE,
        DYNAMIC,
        KINEMATIC,
        STATIC,
        CHARACTER,
        INHERITED
    };

    class Scene;
    class GameObject
    {
    public:
        friend class Scene;
        friend class PhysicsComponent;
        friend class PlayerControllerComponent;

        virtual ~GameObject();
        virtual void Init();
        virtual void PreUpdate(float _deltaTime);
        virtual void Update(float _deltaTime);
        virtual void LateUpdate(float _deltaTime);

        void AddComponent(Component* _component);
        void MarkForDestroy();

        static GameObject* LoadGLTF(const std::string& _path);

        // getters/setters
        const std::string& GetName() const;
        void SetName(const std::string& _name);

        bool SetParent(GameObject* _parent);
        GameObject* GetParent() const;

        GameObject* FindObjectByName(const std::string& _name, bool _searchDescendants = false);
        std::vector<GameObject*> FindObjectsByName(const std::string& _name, bool _searchDescendants = false);
        const std::vector<std::unique_ptr<GameObject>>& GetChildren() const;

        Scene* GetScene() const;

        void SetAlive(bool _alive);
        bool IsAlive() const;

        void SetActive(bool _active);
        bool IsActive() const;

        // pos/rot/scale set/get
        vec3 GetPosition() const;
        vec3 GetWorldPosition() const;
        void SetPosition(const vec3& _pos);
        void SetWorldPosition(const vec3& _pos);

        quat GetRotation() const;
        void SetRotation(const quat& _rot);
        quat GetWorldRotation() const;
        void SetWorldRotation(const quat& _rot);

        const vec3& GetScale() const;
        void SetScale(const vec3& _scale);
        vec3 GetWorldLossyScale() const;

        // Transform get/set
        mat4 GetLocalTransform() const;
        mat4 GetWorldTransform() const;

    protected:
        GameObject();

        PhysicsOwnership GetPhysicsOwnership() const;
        void SetWorldPositionInternal(const vec3& _pos);
        void SetWorldRotationInternal(const quat& _rot);
        void SetScaleInternal(const vec3& _scale);

        bool m_ticked = false;
        bool m_alive = true;
        bool m_active = true;
        std::string m_name;
        GameObject* m_parent = nullptr;
        Scene* m_scene = nullptr;

        // Hierarchy
        std::vector<std::unique_ptr<GameObject>> m_children;
        std::vector<std::unique_ptr<Component>> m_components;
        PhysicsOwnership m_physicsOwnership = PhysicsOwnership::NONE;
        PhysicsOwnership m_parentPhysicsOwnership = PhysicsOwnership::NONE;

        // Object Transform Values
        vec3 m_position = vec3(0.0f);
        quat m_rotation = quat(1.0f, 0.0f, 0.0f, 0.0f);
        vec3 m_scale    = vec3(1.0f);

    private:
        void FindObjectsByName(const std::string& _name, std::vector<GameObject*>& _result);

    public:
        // Templates ---------------------------------------------------------------------------------------------------

        /**
         * @brief Searches and returns a component of the game-object
         * @tparam T Component type you're searching for
         * @return Pointer to the componet
         */
        template<ComponentType T>
        T* FindComponentByType()
        {
            sizeT typeID = Component::StaticTypeID<T>();

            for (auto& component : m_components)
            {
                if (component->GetTypeID() == typeID)
                {
                    return static_cast<T*>(component.get());
                }
            }
            return nullptr;
        }

        /**
         * @brief Searches and returns all component of the type
         *        given in the GameObject
         * @tparam T Component type you're searching for
         * @return vector of pointers to the componet
         */
        template<ComponentType T>
        std::vector<T*> FindComponentsByType()
        {
            std::vector<T*> result;
            sizeT typeID = Component::StaticTypeID<T>();
            for (auto& component : m_components)
            {
                if (component->GetTypeID() == typeID)
                {
                    result.push_back(static_cast<T*>(component.get()));
                }
            }
            return result;
        }

        template<GameObjectType T>
        T* FindObjectByType(bool _searchDescendants = false)
        {
            if (T* typed = dynamic_cast<T*>(this))
            {
                return typed;
            }

            if (!_searchDescendants)
            {
                for (auto& child : m_children)
                {
                    if (T* res = dynamic_cast<T*>(child.get()))
                    {
                        return res;
                    }
                }

                return nullptr;
            }

            for (auto& child : m_children)
            {
                if (T* res = child->FindObjectByType<T>(true))
                {
                    return res;
                }
            }

            return nullptr;
        }

        template<GameObjectType T>
        std::vector<T*> FindObjectsByType(bool _searchDescendants = false)
        {
            std::vector<T*> result {};

            if (_searchDescendants)
            {
                FindObjectsByType<T>(result);
            }
            else
            {
                if (T* typed = dynamic_cast<T*>(this))
                {
                    result.push_back(typed);
                }

                for (auto& child : m_children)
                {
                    if (T* res = dynamic_cast<T*>(child.get()))
                    {
                        result.push_back(res);
                    }
                }
            }

            return result;
        }

    private:

        template<GameObjectType T>
        void FindObjectsByType(std::vector<T*>& _result)
        {
            if (T* typed = dynamic_cast<T*>(this))
            {
                _result.push_back(typed);
            }

            for (auto& child : m_children)
            {
                child->FindObjectsByType<T>(_result);
            }
        }
    };
}


















