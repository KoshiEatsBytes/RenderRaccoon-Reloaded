
#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include "KinematicCharacterController.h"

#include "BtConv.hpp"
#include "Engine.h"


namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    KinematicCharacterController::KinematicCharacterController(float _radius, float _height)
        : m_height(_height), m_radius(_radius)
    {
        BuildGhostAndController(vec3(0.0f));
    }

    KinematicCharacterController::~KinematicCharacterController()
    {
        TearDown();
    }

    void KinematicCharacterController::SetWalkVelocity(const vec3 &_velocity, float _duration)
    {
        m_controller->setVelocityForTimeInterval(BtConv::ToBt(_velocity), _duration);
    }

    void KinematicCharacterController::Jump(const vec3& _direction)
    {
        // if on ground we can jump
        if (m_controller->onGround())
        {
            m_controller->jump(BtConv::ToBt(_direction));
        }
    }

    bool KinematicCharacterController::OnGround() const
    {
        return m_controller->onGround();
    }

    /**
     * Rebuilds the character controller with the new radius and height
     *
     * @param _radius New Radius
     * @param _height New Height
     */
    void KinematicCharacterController::Resize(float _radius, float _height)
    {
        vec3 currentPos = m_ghost ? GetPosition() : vec3(0.0f);

        TearDown();

        m_radius = _radius;
        m_height = _height;

        BuildGhostAndController(currentPos);
    }

    void KinematicCharacterController::SetPosition(const vec3& _pos)
    {
        if (!m_ghost) return;

        btTransform tr = m_ghost->getWorldTransform();
        tr.setOrigin(BtConv::ToBt(_pos));
        m_ghost->setWorldTransform(tr);
        // wake up internal controller
        m_controller->warp(BtConv::ToBt(_pos));
    }

    vec3 KinematicCharacterController::GetPosition() const
    {
        return BtConv::FromBt(m_ghost->getWorldTransform().getOrigin());
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    void KinematicCharacterController::BuildGhostAndController(const vec3& _startPos)
    {
        auto world = Engine::GetInstance().GetPhysicsManager().GetWorld();

        // frees pointer if not already free
        m_controller.reset();
        m_ghost.reset();
        m_capsule.reset();

        m_capsule = std::make_unique<btCapsuleShape>(m_radius, m_height);

        btTransform startTr;
        startTr.setIdentity();
        startTr.setOrigin(BtConv::ToBt(_startPos));

        m_ghost = std::make_unique<btPairCachingGhostObject>();
        m_ghost->setWorldTransform(startTr);
        m_ghost->setCollisionShape(m_capsule.get());
        m_ghost->setCollisionFlags(m_ghost->getCollisionFlags() | btCollisionObject::CF_CHARACTER_OBJECT);

        m_controller = std::make_unique<btKinematicCharacterController>(m_ghost.get(), m_capsule.get(), m_stepHeight);
        m_controller->setGravity(world->getGravity());
        m_controller->setMaxSlope(btRadians(m_maxSlope));

        // Add character controller to the world
        world->addCollisionObject(
            m_ghost.get(),
            btBroadphaseProxy::CharacterFilter,
            btBroadphaseProxy::AllFilter & ~btBroadphaseProxy::SensorTrigger
            );
        world->addAction(m_controller.get());
    }

    void KinematicCharacterController::TearDown()
    {
        auto world = Engine::GetInstance().GetPhysicsManager().GetWorld();

        if (m_controller)
        {
            world->removeAction(m_controller.get());
        }
        if (m_ghost)
        {
            world->removeCollisionObject(m_ghost.get());
        }
    }
}
