
#include "GameObject.h"

#include "Engine.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Helpers/Printer.hpp"
#include "GLTFTypes.h"

namespace RR
{
    // PROTECTED -------------------------------------------------------------------------------------------------------

    GameObject::GameObject()
    = default;

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    GameObject::~GameObject()
    = default;

    void GameObject::Update(const float _deltaTime)
    {
        // Update each component
        for (const auto& component : m_components)
        {
            component->Update(_deltaTime);
        }

        // Update each child, or destroy marked
        for (auto it = m_children.begin(); it != m_children.end();)
        {
            if ((*it)->IsAlive())
            {
                (*it)->Update(_deltaTime);
                ++it;
            }
            else
            {
                // Erase it from list, ptr gets freed when out of scope.
                it = m_children.erase(it);
            }
        }
    }

    /**
     * @brief Adds a component to the current GameObject
     * @param _component Ptr to the component you want to add
     */
    void GameObject::AddComponent(Component* _component)
    {
        m_components.emplace_back(_component);
        _component->m_owner = this;
    }

    void GameObject::MarkForDestroy()
    {
        m_isAlive = false;
    }

    GameObject* GameObject::LoadGLTF(const std::string& _path)
    {
        auto& fileSys = Engine::GetInstance().GetFileSystem();

        auto contents = fileSys.LoadAssetFileText(_path);

        if (contents.empty())
        {
            Warn("[GAME-OBJECT - LOADING - GLTF] File not found: '", _path, "'");
            return nullptr;
        }

        cgOptions options = {};
        cgData* data = nullptr;
        cgResult res = cgltf_parse(&options, contents.data(), contents.size(), &data);

        // if prase unsuccessful
        if (res != cgltf_result_success)
        {
            Warn("[GAME-OBJECT - LOADING - GLTF] GLTF File was not parsed successfully at: '", _path, "'");
            return nullptr;
        }

        auto fullPath = fileSys.GetAssetFolder() / _path;
        auto fullFolderPath = fullPath.remove_filename();
        auto relativeFolderPath = fSysPath(_path).remove_filename();

        // Load binary buffer
        res = cgltf_load_buffers(&options, data, fullFolderPath.string().c_str());

        // check if ok
        if (res != cgltf_result_success)
        {
            Warn("[GAME-OBJECT - LOADING - GLTF] GLTF failed to load buffer successfully of file at path: '", _path, "'");
            cgltf_free(data);
            return nullptr;
        }

        



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

    GameObject* GameObject::GetParent()
    {
        return m_parent;
    }

    bool GameObject::IsAlive() const
    {
        return m_isAlive;
    }

    const vec3& GameObject::GetPosition() const
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
        m_position = _pos;
    }

    const quat& GameObject::GetRotation() const
    {
        return m_rotation;
    }

    void GameObject::SetRotation(const quat& _rot)
    {
        m_rotation = _rot;
    }

    const vec3& GameObject::GetScale() const
    {
        return m_scale;
    }

    void GameObject::SetScale(const vec3& _scale)
    {
        m_scale = _scale;
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
}
