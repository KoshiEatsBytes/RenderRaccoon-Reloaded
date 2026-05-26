
#include "Scene.h"

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

    void Scene::SetParent(GameObject *obj, GameObject *_parent)
    {

    }
}



















