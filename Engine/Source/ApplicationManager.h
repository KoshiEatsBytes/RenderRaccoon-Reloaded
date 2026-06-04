
#pragma once
#include <memory>

#include "Helpers/Printer.hpp"
#include "Scene/Scene.h"
#include "ISubSystem.h"

namespace RR
{
    class Engine;
    class ApplicationManager
    {
        ApplicationManager();
        ~ApplicationManager();

    public:
        // Owned by engine
        friend class Engine;
        ApplicationManager(const ApplicationManager&) = delete;
        ApplicationManager& operator=(const ApplicationManager&) = delete;
        ApplicationManager(ApplicationManager&&) noexcept = delete;
        ApplicationManager& operator=(ApplicationManager&&) noexcept = delete;

        void UnloadCurrentScene();

        GameObject* GetSceneCamera() const;
        Scene* GetActiveScene() const;

    private:
        // Engine hooks
        bool Init();
        void PreUpdate(float _deltaTime) const;
        void Update(float _deltaTime) const;
        void LateUpdate(float _deltaTime);
        void Destroy();

        void LoadPendingScene();

        bool m_pastInitialization = false;
        std::vector<std::unique_ptr<ISubSystem>> m_subSystems;

        std::unique_ptr<Scene>                   m_activeScene;
        std::function<std::unique_ptr<Scene>()>  m_pendingScene;

    public:

        /**
         * @brief Queues up a scene to load next end of frame
         * @tparam T Scene to load
         */
        template <SceneType T>
        void RequestSceneLoad()
        {
            // This lambda represents what will happen LATER
            m_pendingScene = []() -> std::unique_ptr<Scene>
            {
                return std::make_unique<T>();
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

        /**
         * @brief Returns a subsystem
         * @tparam T Type of the subsystem you're tring to retrieve
         * @return ptr to the subsystem, nullptr if not found
         */
        template <SubSystemType T>
        T* GetSubSystem()
        {
            for (auto& SubSys : m_subSystems)
            {
                if (SubSys->IsA<T>())
                {
                    return static_cast<T*>(SubSys.get());
                }
            }

            return nullptr;
        }
    };
}


