
#include "Scene.h"

#include <algorithm>

#include "Helpers/Printer.hpp"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    Scene::Scene()
    = default;

    Scene::~Scene()
    = default;

    void Scene::Update(float _deltaTime)
    {
        // Update each GO, delete unalive ones
        for (auto it = m_objects.begin(); it != m_objects.end();)
        {
            if ((*it)->IsAlive())
            {
                (*it)->Update(_deltaTime);
                ++it;
            }
            else
            {
                it = m_objects.erase(it);
            }
        }
    }

    void Scene::Clear()
    {
        m_objects.clear();
    }

    GameObject* Scene::CreateObject(const std::string& _name, GameObject* _parent)
    {
        auto obj = new GameObject();

        obj->SetName(_name);
        SetParent(obj, _parent);
        return obj;
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

    GameObject* Scene::GetMainCamera()
    {
        return m_mainCamera;
    }
}



















