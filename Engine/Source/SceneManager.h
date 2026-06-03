
#pragma once
#include <memory>

#include "Engine.h"
#include "Helpers/Printer.hpp"
#include "Scene/Scene.h"
#include "ISubSystem.h"

namespace RR
{
    class SceneManager
    {
        SceneManager();
        ~SceneManager();

    public:
        // Owned by engine
        friend class Engine;
        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;
        SceneManager(SceneManager&&) noexcept = delete;
        SceneManager& operator=(SceneManager&&) noexcept = delete;

        void UnloadCurrentScene();

        GameObject* GetSceneCamera() const;
        Scene* GetActiveScene() const;

    private:
        // Engine hooks
        bool Init();
        void PreUpdate(float _deltaTime) const;
        void Update(float _deltaTime) const;
        void LateUpdate(float _deltaTime) const;
        void Destroy();

        void LoadPendingScene();

        bool m_pastInitialization = false;
        std::vector<std::unique_ptr<ISubSystem>> m_subSystems;

        std::unique_ptr<Scene>                  m_activeScene;
        std::function<std::unique_ptr<Scene>()> m_pendingScene;

    public:

        /**
         * @brief Queues up a scene to load next end of frame
         * @tparam T Scene to load
         */
        template <SceneType T>
        void RequestSceneLoad()
        {
            // This lambda represents what will happen LATER
            m_pendingScene = [this]() -> std::unique_ptr<Scene>
            {
                std::unique_ptr<T> newScene = std::make_unique<T>();

                if (!newScene->OnLoad(&Engine::GetInstance().GetAppData()))
                {
                    Error("[SCENE - LOADING - INITIALIZATION] Scene '", newScene->m_name, "' "
                          "has failed to initialize! this scene will not be loaded.");
                    return nullptr;
                }

                Success("[SCENE - LOADING] Scene '", newScene->m_name, "' Loaded correctly!");

                return newScene;
            };
        }

        /**
         * @brief Adds a subsystem to the scene manager
         * @tparam T Type of the subSystem
         */
        template <SubSystemType T>
        void AddSubSystem()
        {
            if (m_pastInitialization)
            {
                Warn("[SUBSYSTEM - ADD] Trying to add a SubSystem AFTER initialization");
                return;
            }

            if (GetSubSystem<T>() != nullptr)
            {
                Warn("[SUBSYSTEM - ADD] Trying to add already existing SubSystem, discarding");
                return;
            }

            m_subSystems.push_back(std::make_unique<T>());
        }

        template <SubSystemType T>
        T* GetSubSystem()
        {
            // TODO:: IMPLEMENT SIMPLE RTTI (SAME AS COMPONENTS) FOR SUBSYSTEMS
            return nullptr;
        }
    };
}


