
#include "ApplicationManager.h"

#include "../../Source/Game.h"
#include "ISubSystem.h"


namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    ApplicationManager::ApplicationManager()
    = default;

    ApplicationManager::~ApplicationManager()
    = default;

    void ApplicationManager::UnloadCurrentScene()
    {
        if (!m_activeScene) return;

        m_activeScene->OnDestroy();
        m_activeScene.reset();
    }

    GameObject* ApplicationManager::GetSceneCamera() const
    {
        if (m_activeScene)
        {
            return m_activeScene->GetMainCamera();
        }

        return nullptr;
    }

    Scene* ApplicationManager::GetActiveScene() const
    {
        return m_activeScene.get();
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    bool ApplicationManager::Init()
    {
        // blocks subsystems from adding subsystems at init
        m_pastInitialization = true;

        // Subsystem should be added in MAIN via Engine::GetInstance().GetAppManager().AddSubSystem<T>()
        for (auto& subSystem : m_subSystems)
        {
            if (subSystem)
            {
                if (!subSystem->Init())
                {
                    Error("[SUBSYSTEMS - INITIALIZATION] Subsystem has failed to initialize.");
                    return false;
                }

                //subSystem->SetAppData(&Engine::GetInstance().GetAppData());
            }
        }

        // Erase nullptrs
        std::erase_if(m_subSystems, [](const auto& ptr) { return !ptr; });

        // Default scene
        RequestSceneLoad<Game>();

        return true;
    }

    void ApplicationManager::PreUpdate(float _deltaTime) const
    {
        for (auto& subSystem : m_subSystems)
        {
            subSystem->PreUpdate(_deltaTime);
        }

        if (m_activeScene) m_activeScene->PreUpdateInternal(_deltaTime);
    }

    void ApplicationManager::Update(float _deltaTime) const
    {
        for (auto& subSystem : m_subSystems)
        {
            subSystem->Update(_deltaTime);
        }

        if (m_activeScene) m_activeScene->UpdateInternal(_deltaTime);
    }

    void ApplicationManager::LateUpdate(float _deltaTime)
    {
        for (auto& subSystem : m_subSystems)
        {
            subSystem->LateUpdate(_deltaTime);
        }

        if (m_activeScene) m_activeScene->LateUpdateInternal(_deltaTime);

        // Load pending scene
        LoadPendingScene();
    }

    void ApplicationManager::Destroy()
    {
        UnloadCurrentScene();

        for (auto& subSystem : m_subSystems)
        {
            subSystem->Destroy();
        }

        m_subSystems.clear();
    }

    void ApplicationManager::LoadPendingScene()
    {
        if (!m_pendingScene) return;

        std::unique_ptr<Scene> newScene = m_pendingScene();
        m_pendingScene = nullptr;

        if (!newScene) return;

        // Stash old scene
        std::unique_ptr<Scene> oldScene = std::move(m_activeScene);

        // Install, then init
        m_activeScene = std::move(newScene);

        if (m_activeScene->OnLoad())
        {
            Success("[SCENE - LOADING] Scene '", m_activeScene->m_name, "' loaded correctly!");
            if (oldScene) oldScene->OnDestroy();
        }
        else
        {
            Error("[SCENE - LOADING] Scene '", m_activeScene->m_name, "' failed to initialize! reverting.");
            m_activeScene->OnDestroy();

            // Roll back if fail
            m_activeScene = std::move(oldScene);
        }
    }
}
