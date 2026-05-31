
#pragma once
#include <string>
#include <vector>
#include <memory>

#include "Concepts.h"
#include "Types.h"
#include "Component.h"

namespace RR
{
    class Scene;
    class GameObject
    {
    public:
        friend class Scene;
        virtual ~GameObject();
        virtual void Update(float _deltaTime);

        void AddComponent(Component* _component);
        void MarkForDestroy();

        static GameObject* LoadGLTF(const std::string& _path);

        // getters/setters
        const std::string& GetName() const;
        void SetName(const std::string& _name);

        bool SetParent(GameObject* _parent);
        GameObject* GetParent() const;
        GameObject *GetChildByName(const std::string& _name);

        Scene* GetScene() const;

        void SetAlive(bool _alive);
        bool IsAlive() const;

        void SetActive(bool _active);
        bool IsActive() const;

        // pos/rot/scale set/get
        const vec3& GetPosition() const;
        vec3 GetWorldPosition() const;
        void SetPosition(const vec3& _pos);

        const quat& GetRotation() const;
        void SetRotation(const quat& _rot);

        const vec3& GetScale() const;
        void SetScale(const vec3& _scale);

        // Transform get/set
        mat4 GetLocalTransform() const;
        mat4 GetWorldTransform() const;

    protected:
        GameObject();

    private:
        bool m_alive = true;
        bool m_active = true;
        std::string m_name;
        GameObject* m_parent = nullptr;
        Scene* m_scene = nullptr;

        // Hierarchy
        std::vector<std::unique_ptr<GameObject>> m_children;
        std::vector<std::unique_ptr<Component>> m_components;

        // Object Transform Values
        vec3 m_position = vec3(0.0f);
        quat m_rotation = quat(1.0f, 0.0f, 0.0f, 0.0f);
        vec3 m_scale    = vec3(1.0f);

    public:
        // Templates ---------------------------------------------------------------------------------------------------

        /**
         * @brief Searches and returns a component of the game-object
         * @tparam T Component type you're searching for
         * @return Pointer to the componet
         */
        template<ComponentType T>
        T* GetComponent()
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
    };
}

