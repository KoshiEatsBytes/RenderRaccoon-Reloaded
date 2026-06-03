
#pragma once
#include <string>
#include <vector>
#include <memory>

#include "Helpers/Concepts.h"
#include "Helpers/Common.h"
#include "Helpers/ApplicationData.h"
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
        friend class SceneManager;

        Scene();
        Scene(const std::string& _sceneName);
        virtual ~Scene();

        void Clear();

        GameObject* CreateObject(const std::string& _name, GameObject* _parent = nullptr);
        GameObject* FindObjectByName(const std::string& _name, bool _searchDescendants = false);
        std::vector<GameObject*> FindObjectsByName(const std::string& _name, bool _searchDescendants = false) const;

        void EnqueueDestroy(GameObject* _object);
        void EnqueueSpawn(GameObject* _object, GameObject* _parent);

        // Get/Sets
        bool SetParent(GameObject* _obj, GameObject* _parent);
        void SetMainCamera(GameObject* _camera);
        GameObject* GetMainCamera() const;
        std::vector<LightData> GetLights() const;

    protected:
        // Scene hooks
        virtual bool Init()                         = 0;
        virtual void PreUpdate(float _deltaTime)    = 0;
        virtual void Update(float _deltaTime)       = 0;
        virtual void LateUpdate(float _deltaTime)   = 0;
        virtual void Destroy()                      = 0;

        std::string m_name = "noName";
        ApplicationData* m_appData = nullptr;

    private:
        // internal hooks
        void PreUpdateInternal(float _deltaTime);
        void UpdateInternal(float _deltaTime);
        void LateUpdateInternal(float _deltaTime);

        void FlushPendingChanges();
        void ProcessDestroyQueue();
        void ProcessSpawnQueue();

        bool OnLoad(ApplicationData* _appData);
        void OnDestroy();

        static void CollectLightsRecursive(GameObject* _obj, std::vector<LightData>& _out);

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

        /**
         * @brief Returns pointer to the object of the type requested, nullptr if not found
         * @tparam T Type of the object to retrieve
         * @param _searchDescendants Expands search to children of children
         * @return ptr to the GO of that type
         */
        template<GameObjectType T>
        T* FindObjectByType(bool _searchDescendants = false)
        {
            if (!_searchDescendants)
            {
                for (auto& obj : m_objects)
                {
                    if (obj->IsA<T>())
                    {
                        return static_cast<T*>(obj.get());;
                    }
                }
                return nullptr;
            }

            for (auto& obj : m_objects)
            {
                if (T* res = obj->FindObjectByType<T>(true))
                {
                    return res;
                }
            }

            return nullptr;
        }

        /**
         * @brief Retruns vector of pointers to GO of the type quested
         * @tparam T Type of the GO to search
         * @param _searchDescendants Expands search to children of children
         * @return vector of ptr of the GOs
         */
        template<GameObjectType T>
        std::vector<T*> FindObjectsByType(bool _searchDescendants = false)
        {
            std::vector<T*> result {};

            if (!_searchDescendants)
            {
                for (auto& obj : m_objects)
                {
                    if (obj->IsA<T>())
                    {
                        result.push_back(static_cast<T*>(obj.get()));
                    }
                }

                return result;
            }

            for (auto& obj : m_objects)
            {
                obj->FindObjectsByType<T>(result);
            }

            return result;
        }
    };
}

