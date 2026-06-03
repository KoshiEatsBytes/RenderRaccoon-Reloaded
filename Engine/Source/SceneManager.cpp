
#include "SceneManager.h"

#include "../../Source/Game.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    SceneManager::SceneManager()
    = default;

    SceneManager::~SceneManager()
    = default;

    void SceneManager::UnloadCurrentScene()
    {
        m_activeScene->OnDestroy();
        m_activeScene.reset();
    }

    GameObject* SceneManager::GetSceneCamera() const
    {
        if (m_activeScene)
        {
            return m_activeScene->GetMainCamera();
        }

        return nullptr;
    }

    Scene* SceneManager::GetActiveScene() const
    {
        return m_activeScene.get();
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    bool SceneManager::Init()
    {
        for (auto& subSystem : m_subSystems)
        {
            if (subSystem)
            {
                if (!subSystem->Init())
                {
                    Error("[SUBSYSTEMS - INITIALIZATION] Subsystem has failed to initialize.");
                    return false;
                }
            }
        }

        // Erase nullptrs
        std::erase_if(m_subSystems, [](const auto& ptr) { return !ptr; });

        // Default scene
        RequestSceneLoad<Game>();
        m_pastInitialization = true;

        return true;
    }

    void SceneManager::PreUpdate(float _deltaTime) const
    {
        for (auto& subSystem : m_subSystems)
        {
            subSystem->PreUpdate(_deltaTime);
        }

        if (m_activeScene) m_activeScene->PreUpdate(_deltaTime);
    }

    void SceneManager::Update(float _deltaTime) const
    {
        for (auto& subSystem : m_subSystems)
        {
            subSystem->Update(_deltaTime);
        }

        if (m_activeScene) m_activeScene->Update(_deltaTime);
    }

    void SceneManager::LateUpdate(float _deltaTime) const
    {
        for (auto& subSystem : m_subSystems)
        {
            subSystem->LateUpdate(_deltaTime);
        }

        if (m_activeScene) m_activeScene->LateUpdate(_deltaTime);
    }

    void SceneManager::Destroy()
    {
        UnloadCurrentScene();

        for (auto& subSystem : m_subSystems)
        {
            subSystem->Destroy();
        }
    }

    void SceneManager::LoadPendingScene()
    {
        // check if any scene needs to be loaded
        if (m_pendingScene)
        {
            std::unique_ptr<Scene> newScene = m_pendingScene();
            m_pendingScene = nullptr;

            //if new scene is valid
            if (newScene)
            {
                // Destroy old scene and free memory
                m_activeScene->OnDestroy();

                // Load new scene, hooray!
                m_activeScene = std::move(newScene);
            }
        }
    }
}
