
#pragma once
#include <string>
#include <vector>
#include <memory>

#include "Common.h"
#include "GameObject.h"
#include "Concepts.h"

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

        std::vector<LightData> CollectLights();

        // Get/Sets
        bool SetParent(GameObject* _obj, GameObject* _parent);
        void SetMainCamera(GameObject* _camera);
        GameObject* GetMainCamera() const;

    private:
        void CollectLightsRecursive(GameObject* _obj, std::vector<LightData>& _out);

        std::vector<std::unique_ptr<GameObject>> m_objects;
        GameObject* m_mainCamera = nullptr;

    public:
        // Templates ---------------------------------------------------------------------------------------------------

        /**
         * @brief Uses polymorphism to instantiate the desired gameObject
         * @tparam T Type of the GO
         * @param _name Name of the GO
         * @param _parent Parent of the GO
         * @return pointer to the GO
         */
        template<GameObjectType T>
        T* CreateObject(const std::string& _name, GameObject* _parent = nullptr)
        {
            auto obj = new T();

            obj->SetName(_name);
            SetParent(obj, _parent);
            obj->m_scene = this;
            return obj;
        }
    };
}

