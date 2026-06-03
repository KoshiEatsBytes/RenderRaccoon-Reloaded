
#pragma once
#include <functional>
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
        using StepCallBack = std::function<void()>;
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

        void RegisterPreStepCallback(const StepCallBack& _callback);
        void RegisterPostStepCallback(const StepCallBack& _callback);

        btDiscreteDynamicsWorld* GetWorld() const;
        float GetInterpolationAlpha() const;

        // Constant time step
        static constexpr float FixedTimeStep = 1.0f / 60.0f;

    private:
        std::unique_ptr<btGhostPairCallback> m_ghostPairCallback;
        std::unique_ptr<btBroadphaseInterface> m_broadphase;
        std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfig;
        std::unique_ptr<btCollisionDispatcher> m_dispatcher;
        std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
        std::unique_ptr<btDiscreteDynamicsWorld> m_world;

        // Interpolation data for position at inconsistent framerate
        float m_accumulator = 0.0f;
        std::vector<StepCallBack> m_preStepCallbacks;
        std::vector<StepCallBack> m_postStepCallbacks;
    };
}
