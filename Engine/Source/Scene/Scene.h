
#pragma once
#include <string>
#include <vector>
#include <memory>

#include "GameObject.h"

namespace RR
{
    class Scene
    {
    public:
        Scene();
        ~Scene();

        void Update(float _deltaTime);
        void Clear();

        GameObject* CreateObject(const std::string& _name, GameObject* _parent = nullptr);

        template<typename T, typename = typename std::enable_if<std::is_base_of_v<GameObject, T>>>
        T* CreateObject(const std::string& _name, GameObject* _parent = nullptr)
        {
            auto obj = new T();

            obj->SetName(_name);
            SetParent(obj, _parent);
            return obj;
        }

        bool SetParent(GameObject* _obj, GameObject* _parent);

    private:
        std::vector<std::unique_ptr<GameObject>> m_objects;
    };
}

