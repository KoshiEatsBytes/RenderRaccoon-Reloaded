
#pragma once
#include <string>
#include <vector>
#include <memory>

#include "Helpers/Concepts.h"
#include "Helpers/Common.h"
#include "GameObject.h"

namespace RR
{
    /**
     * Container for game-objects spawned mid-frame
     */
    struct PendingSpawn
    {
        GameObject* object = nullptr;
        GameObject* parent = nullptr;
    };

    class Scene
    {
    public:
        Scene();
        ~Scene();

        void PreUpdate(float _deltaTime);
        void Update(float _deltaTime) const;
        void LateUpdate(float _deltaTime);
        void Clear();

        void EnqueueDestroy(GameObject* _object);
        void EnqueueSpawn(GameObject* _object, GameObject* _parent);

        GameObject* CreateObject(const std::string& _name, GameObject* _parent = nullptr);

        std::vector<LightData> CollectLights();

        // Get/Sets
        bool SetParent(GameObject* _obj, GameObject* _parent);
        void SetMainCamera(GameObject* _camera);
        GameObject* GetMainCamera() const;

    private:
        void FlushPendingChanges();
        void ProcessDestroyQueue();
        void ProcessSpawnQueue();

        void CollectLightsRecursive(GameObject* _obj, std::vector<LightData>& _out);

        std::vector<std::unique_ptr<GameObject>> m_objects;
        GameObject* m_mainCamera = nullptr;

        // Deferred queue vectors
        bool m_sceneStarted = false;
        std::vector<PendingSpawn>  m_spawnQueue;
        std::vector<GameObject*>   m_destroyQueue;

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

            // If scene has started defer init and parenting
            if (m_sceneStarted)
            {
                EnqueueSpawn(obj, _parent);
                return obj;
            }

            SetParent(obj, _parent);
            obj->m_scene = this;
            obj->Init();

            return obj;
        }
    };
}

