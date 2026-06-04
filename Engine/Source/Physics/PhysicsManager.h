
#pragma once
#include <functional>
#include <memory>
#include <vector>

class btGhostPairCallback;
class btDbvtBroadphase;
class btDefaultCollisionConfiguration;
class btCollisionDispatcherMt;
class btConstraintSolverPoolMt;
class btSequentialImpulseConstraintSolverMt;
class btDiscreteDynamicsWorldMt;

namespace RR
{
    class Scene;
    class RigidBody;
    class PhysicsManager
    {
    public:
        PhysicsManager();
        ~PhysicsManager();

        using StepCallBack   = std::function<void()>;
        using CallbackHandle = std::size_t;
        static constexpr CallbackHandle InvalidCallbackHandle = 0;

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

        CallbackHandle RegisterPreStepCallback(const StepCallBack& _callback);
        CallbackHandle RegisterPostStepCallback(const StepCallBack& _callback);
        void UnregisterPreStepCallback(CallbackHandle _handle);
        void UnregisterPostStepCallback(CallbackHandle _handle);

        btDiscreteDynamicsWorldMt* GetWorld() const;
        float GetInterpolationAlpha() const;

        // Constant time step
        static constexpr float FixedTimeStep = 1.0f / 60.0f;

    private:
        std::unique_ptr<btGhostPairCallback> m_ghostPairCallback;

        // bt physics world config
        std::unique_ptr<btDbvtBroadphase> m_broadphase;
        std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfig;
        std::unique_ptr<btCollisionDispatcherMt> m_dispatcher;
        std::unique_ptr<btConstraintSolverPoolMt> m_solverPool;
        std::unique_ptr<btSequentialImpulseConstraintSolverMt> m_solverMT;
        std::unique_ptr<btDiscreteDynamicsWorldMt> m_world;

        // Interpolation data for position at inconsistent framerate
        float m_accumulator = 0.0f;

        // Data for callback entries
        struct CallBackEntry
        {
            CallbackHandle  handle;
            StepCallBack    callBack;
        };
        std::vector<CallBackEntry> m_preStepCallbacks;
        std::vector<CallBackEntry> m_postStepCallbacks;
        CallbackHandle m_nextHandle = 1;
    };
}
