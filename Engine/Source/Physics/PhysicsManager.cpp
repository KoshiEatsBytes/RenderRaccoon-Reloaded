
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

#include "PhysicsManager.h"

#include "RigidBody.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "Helpers/Printer.hpp"
#include "Helpers/Types.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    PhysicsManager::PhysicsManager()
    = default;

    PhysicsManager::~PhysicsManager()
    = default;

    bool PhysicsManager::Init()
    {
        // Init bullet components
        m_broadphase        = std::make_unique<btDbvtBroadphase>();
        m_collisionConfig   = std::make_unique<btDefaultCollisionConfiguration>();
        m_dispatcher        = std::make_unique<btCollisionDispatcher>(m_collisionConfig.get());
        m_solver            = std::make_unique<btSequentialImpulseConstraintSolver>();

        // Silences warning for non dynamic-dynamic collisions
        m_dispatcher->setDispatcherFlags(
            m_dispatcher->getDispatcherFlags()
            | btCollisionDispatcher::CD_STATIC_STATIC_REPORTED);

        // Create collision world with bullet components
        m_world = std::make_unique<btDiscreteDynamicsWorld>(
            m_dispatcher.get(),
            m_broadphase.get(),
            m_solver.get(),
            m_collisionConfig.get()
            );

        m_ghostPairCallback = std::make_unique<btGhostPairCallback>();
        m_world->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(m_ghostPairCallback.get());

        // Set earth's gravity as default
        m_world->setGravity(btVec3(0.f, -9.81f, 0.f));
        return true;
    }

    void PhysicsManager::Update(float _deltaTime)
    {
        constexpr int maxStepsPerFrame = 4;

        m_accumulator += _deltaTime;
        m_accumulator = std::min(m_accumulator, maxStepsPerFrame * FixedTimeStep);

        while (m_accumulator >= FixedTimeStep)
        {
            // Pre and post step callbacks to interpolate position of characters
            for (auto& cb : m_preStepCallbacks)  cb();
            m_world->stepSimulation(FixedTimeStep, 1, FixedTimeStep);
            for (auto& cb : m_postStepCallbacks) cb();
            m_accumulator -= FixedTimeStep;
        }
    }

    void PhysicsManager::AddRigidbody(RigidBody* _body)
    {
        if (!_body)
        {
            Warn("[RIGIDBODY - ADD] Tried adding INVALID RigidBody to physics scene");
            return;
        }

        if (!m_world)
        {
            Warn("[RIGIDBODY - ADD] Tried adding a RigidBody when the physic scene has not been initialized");
            return;
        }

        if (auto rigidBody = _body->GetBody())
        {
            m_world->addRigidBody(
                rigidBody,
                _body->GetGroup(),
                _body->GetMask()
                );

            _body->SetAddedToWorld(true);
            return;
        }

        Warn("[RIGIDBODY - ADD] Tried adding a RigidBody with a missing m-body");
    }

    void PhysicsManager::RemoveRigidBody(RigidBody* _body)
    {
        if (!_body)
        {
            Warn("[RIGIDBODY - REMOVE] Tried removing INVALID RigidBody to physics scene");
            return;
        }

        if (!m_world)
        {
            Warn("[RIGIDBODY - REMOVE] Tried removing a RigidBody when the physic scene has not been initialized");
            return;
        }

        if (auto rigidBody = _body->GetBody())
        {
            m_world->removeRigidBody(rigidBody);
            _body->SetAddedToWorld(false);
        }
    }

    // Takes a function pointer to a subscriber
    void PhysicsManager::RegisterPreStepCallback(const StepCallBack& _callback)
    {
        m_preStepCallbacks.push_back(_callback);
    }

    // Takes a function pointer to a subscriber
    void PhysicsManager::RegisterPostStepCallback(const StepCallBack& _callback)
    {
        m_postStepCallbacks.push_back(_callback);
    }

    btDiscreteDynamicsWorld* PhysicsManager::GetWorld() const
    {
        return m_world.get();
    }

    float PhysicsManager::GetInterpolationAlpha() const
    {
        return m_accumulator / FixedTimeStep;
    }
}
