
#pragma once
#include <memory>

// bullet library fw dec
class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btGhostPairCallback;

namespace RR
{
    class RigidBody;

    class PhysicsManager
    {
        PhysicsManager();
        ~PhysicsManager();

    public:
        friend class Engine;

        // Delete copy
        PhysicsManager(const PhysicsManager&) = delete;
        PhysicsManager& operator=(const PhysicsManager&) = delete;

        // Delete move
        PhysicsManager(PhysicsManager&&) noexcept = delete;
        PhysicsManager& operator=(PhysicsManager&&) noexcept = delete;

        bool Init();
        void Update(float _deltaTime);

        void AddRigidbody(RigidBody* _body);
        void RemoveRigidBody(RigidBody* _body);

        btDiscreteDynamicsWorld* GetWorld() const;

    private:
        std::unique_ptr<btGhostPairCallback> m_ghostPairCallback;
        std::unique_ptr<btBroadphaseInterface> m_broadphase;
        std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfig;
        std::unique_ptr<btCollisionDispatcher> m_dispatcher;
        std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
        std::unique_ptr<btDiscreteDynamicsWorld> m_world;
    };
}
