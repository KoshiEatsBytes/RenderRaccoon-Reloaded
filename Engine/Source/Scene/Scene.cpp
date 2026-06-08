
#include "Scene.h"

#include <algorithm>
#include <unordered_set>

#include "Engine.h"
#include "Components/LightComponent.h"
#include "Helpers/Printer.hpp"
#include "Physics/PhysicsManager.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    Scene::Scene(const std::string& _sceneName)
        : m_name(_sceneName), m_appData(Engine::GetInstance().GetAppData())
    {
    }

    Scene::~Scene()
    {
        for (auto& pending : m_spawnQueue)
        {
            delete pending.object;
        }
        m_spawnQueue.clear();
    }

    void Scene::Clear()
    {
        for (auto& pending : m_spawnQueue)
        {
            delete pending.object;
        }

        m_spawnQueue.clear();
        m_destroyQueue.clear();
        m_objects.clear();
        m_sceneStarted = false;
        m_mainCamera = nullptr;
    }

    GameObject* Scene::CreateObject(const std::string& _name, GameObject* _parent)
    {
        auto obj = new GameObject();
        obj->SetName(_name);

        // If scene has started defer init and parenting
        if (m_sceneStarted)
        {
            obj->m_scene = this;
            obj->Init();
            EnqueueSpawn(obj, _parent);
            return obj;
        }

        SetParent(obj, _parent);
        obj->m_scene = this;
        obj->Init();

        return obj;
    }

    GameObject* Scene::FindObjectByName(const std::string& _name, bool _searchDescendants)
    {
        if (!_searchDescendants)
        {
            // If no need to iterate through the childrens, use flat find_if search
            auto it = std::ranges::find_if(m_objects,
            [&_name](const std::unique_ptr<GameObject>& _ptr)
            {
                return _ptr->GetName() == _name;
            });

            if (it != m_objects.end())
            {
                return it->get();
            }

            return nullptr;
        }

        // For children iteration follow same model as in GO
        for (auto& obj : m_objects)
        {
            if (auto res = obj->FindObjectByName(_name, true))
            {
                return res;
            }
        }

        return nullptr;
    }

    std::vector<GameObject*> Scene::FindObjectsByName(const std::string& _name, bool _searchDescendants)
    {
        std::vector<GameObject*> result {};

        if (!_searchDescendants)
        {
            for (auto& obj : m_objects)
            {
                if (obj->GetName() == _name)
                {
                    result.push_back(obj.get());
                }
            }

            return result;
        }

        for (auto& obj : m_objects)
        {
            obj->FindObjectsByName(_name, result);
        }

        return result;
    }

    void Scene::EnqueueDestroy(GameObject* _object)
    {
        m_destroyQueue.push_back(_object);
    }

    void Scene::EnqueueSpawn(GameObject* _object, GameObject* _parent)
    {
        m_spawnQueue.push_back({
            _object,
            _parent
        });
    }

    /**
     * @brief Handles parenting for gameobjects in the scene
     * @param _obj Current object
     * @param _parent new parent of the object (can be left empty)
     * @return if the parenting/unparenting was successful
     */
    bool Scene::SetParent(GameObject* _obj, GameObject* _parent)
    {
        bool result = false;
        auto currentParent = _obj->GetParent();

        // Assign to ROOT
        if (_parent == nullptr)
        {
            // Assign to root object WITH parent
            if (currentParent != nullptr)
            {
                // locate object inside current parent
                auto it = std::find_if(
                    currentParent->m_children.begin(),
                    currentParent->m_children.end(),
                    [_obj](const auto& el)
                    {
                        return el.get() == _obj; // Returns vector pos of matching element
                    });

                // if object has been found inside parent
                if (it != currentParent->m_children.end())
                {
                    // Parents it to scene root and removes it from parent
                    m_objects.push_back(std::move(*it));
                    _obj->m_parent = nullptr;
                    currentParent->m_children.erase(it);
                    result = true;
                }
                else
                {
                    Error("[SCENE - SET PARENT] '", _obj->GetName(), "' not found in parent '",
                        currentParent->GetName(), "' children list - hierarchy corrupted");
                }
            }
            // Object has NO parent
            // 1 - The object is in the scene root
            // 2 - The object has just been created
            else
            {
                // Search in the scene
                auto it = std::find_if(
                    m_objects.begin(),m_objects.end(),
                    [_obj](const auto& el)
                    {
                        return el.get() == _obj;
                    });

                // Object NOT found in root
                if (it == m_objects.end())
                {
                    // Object has just been created, add to scene root
                    std::unique_ptr<GameObject> objHolder(_obj);
                    m_objects.push_back(std::move(objHolder));
                    result = true;
                }
                else
                {
                    Warn("[SCENE - SET PARENT] '", _obj->GetName(), "' is already at scene root");
                }
            }
        }
        // new Parent is NOT NULL - Add as a child of new parent
        else
        {
            // Object already has a parent
            if (currentParent != nullptr)
            {
                // Locate object in current parent list
                auto it = std::find_if(
                    currentParent->m_children.begin(),
                    currentParent->m_children.end(),
                    [_obj](const auto& el)
                    {
                        return el.get() == _obj;
                    });

                // if found - do not allow a child to be parented to one of its own descendants
                if (it != currentParent->m_children.end())
                {
                    bool found = false;
                    auto currentElement = _parent;

                    // Goes through child to check if any match
                    while (currentElement)
                    {
                        if (currentElement == _obj)
                        {
                            found = true;
                            break;
                        }
                        currentElement = currentElement->GetParent();
                    }

                    // No descendant found
                    if (!found)
                    {
                        // parent it to the new parent
                        _parent->m_children.push_back(std::move(*it));
                        _obj->m_parent = _parent;
                        currentParent->m_children.erase(it);
                        result = true;
                    }
                    else
                    {
                        Warn("[SCENE - SET PARENT] Circular dependency detected: '",_obj->GetName(),
                            "' cannot be parented to '", _parent->GetName(), "'");
                    }
                }
                else
                {
                    Error("[SCENE - SET PARENT] '", _obj->GetName(), "' not found in parent '",
                          currentParent->GetName(), "' children list - hierarchy corrupted");
                }
            }
            // Object has NO parent
            // 1 - The object is in the scene root
            // 2 - The object has just been created
            else
            {
                // Search in the scene
                auto it = std::find_if(
                    m_objects.begin(),m_objects.end(),
                    [_obj](const auto& el)
                    {
                        return el.get() == _obj;
                    });

                // If not found object has been just created
                if (it == m_objects.end())
                {
                    std::unique_ptr<GameObject> objHolder(_obj);
                    _parent->m_children.push_back(std::move(objHolder));
                    _obj->m_parent = _parent;
                    result = true;
                }
                // Object *is* in the scene root
                else
                {
                    bool found = false;
                    auto currentElement = _parent;

                    // check again for circular parenting
                    while (currentElement)
                    {
                        if (currentElement == _obj)
                        {
                            found = true;
                            break;
                        }
                        currentElement = currentElement->GetParent();
                    }

                    // If new parent is valid
                    if (!found)
                    {
                        _parent->m_children.push_back(std::move(*it));
                        _obj->m_parent = _parent;
                        m_objects.erase(it);
                        result = true;
                    }
                    else
                    {
                        Warn("[SCENE - SET PARENT] Circular dependency detected: '",_obj->GetName(),
                            "' cannot be parented to '", _parent->GetName(), "'");
                    }
                }
            }
        }

        return result;
    }

    // GETTER / SETTERS ------------------------------------------------------------------------------------------------

    void Scene::SetMainCamera(GameObject* _camera)
    {
        m_mainCamera = _camera;
    }

    GameObject* Scene::GetMainCamera() const
    {
        return m_mainCamera;
    }

    PhysicsManager* Scene::GetPhysicsManager() const
    {
        return m_physicsManager.get();
    }

    /**
     * @brief Iterates through all objects, recursively check if they have a light component
     *        if so, grab its color and world position
     * @return list of all lights
     */
    std::vector<LightData> Scene::GetLights() const
    {
        std::vector<LightData> lights;

        for (auto& obj : m_objects)
        {
            CollectLightsRecursive(obj.get(), lights);
        }

        return lights;
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    void Scene::PreUpdateInternal(float _deltaTime)
    {
        // Set Scene as started for creating objects
        if (!m_sceneStarted) m_sceneStarted = true;

        for (const auto& obj: m_objects)
        {
            obj->PreUpdate(_deltaTime);
        }

        PreUpdate(_deltaTime);
    }

    void Scene::UpdateInternal(float _deltaTime)
    {
        for (const auto& obj: m_objects)
        {
            obj->Update(_deltaTime);

        }

        Update(_deltaTime);
    }

    void Scene::LateUpdateInternal(float _deltaTime)
    {
        for (const auto& obj: m_objects)
        {
            obj->LateUpdate(_deltaTime);

            // Enqueue for destruction any dying object
            if (!obj->IsAlive())
            {
                EnqueueDestroy(obj.get());
            }
        }

        LateUpdate(_deltaTime);
        FlushPendingChanges();
    }

    void Scene::UpdatePhysicsInternal(float _deltaTime) const
    {
        if (m_physicsManager) m_physicsManager->Update(_deltaTime);
    }

    void Scene::CollectLightsRecursive(GameObject* _obj, std::vector<LightData>& _out)
    {
        // check if it has a light component
        if (const auto light = _obj->FindComponentByType<LightComponent>())
        {
            if (!light->IsEnabled()) return;

            // save light source to light vec
            LightData lData;
            lData.color = light->GetColor();
            lData.position = _obj->GetWorldPosition();
            _out.push_back(lData);
        }

        // keep going for each child of child
        for (auto& child : _obj->m_children)
        {
            CollectLightsRecursive(child.get(), _out);
        }
    }

    void Scene::FlushPendingChanges()
    {
        ProcessDestroyQueue();
        ProcessSpawnQueue();
    }

    void Scene::ProcessDestroyQueue()
    {
        auto queue = std::exchange(m_destroyQueue, {});
        if (queue.empty()) return;

        std::unordered_set<GameObject*> toDestroy(queue.begin(), queue.end());
        toDestroy.erase(nullptr);

        // Find objects with NO queued ancestor
        std::vector<GameObject*> roots;
        for (GameObject* obj : toDestroy)
        {
            bool ancestorQueued = false;
            for (GameObject* parent = obj->GetParent(); parent; parent = parent->GetParent())
            {
                if (toDestroy.contains(parent)) { ancestorQueued = true; break; }
            }
            if (!ancestorQueued) roots.push_back(obj);
        }

        // Erase only roots
        for (GameObject* obj : roots)
        {
            if (obj == m_mainCamera) m_mainCamera = nullptr;

            if (auto* parent = obj->GetParent())
            {
                auto& children = parent->m_children;
                auto it = std::ranges::find_if(children,
                    [obj](const auto& p){ return p.get() == obj; });
                if (it != children.end()) children.erase(it);
            }
            else
            {
                auto it = std::ranges::find_if(m_objects,
                    [obj](const auto& p){ return p.get() == obj; });
                if (it != m_objects.end()) m_objects.erase(it);
            }
        }
    }

    void Scene::ProcessSpawnQueue()
    {
        // Process pending objects and creates them in the scene, then init
        for (auto& pending : std::exchange(m_spawnQueue, {}))
        {
            GameObject* object = pending.object;
            GameObject* parent = pending.parent;

            SetParent(object, parent);
        }
    }

    bool Scene::OnLoad()
    {
        m_physicsManager = std::make_unique<PhysicsManager>();

        if (!m_physicsManager->Init())
        {
            Error("[PHYSICS - INITIALIZATION] Failed to initialize RR Physics Engine");
            return false;
        }

        m_sceneStarted = false;
        return Init();
    }

    void Scene::OnDestroy()
    {
        Destroy();
        Clear();
    }
}



















