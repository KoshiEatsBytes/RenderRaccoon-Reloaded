
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

#include "RigidBody.h"
#include "Engine.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    RigidBody::RigidBody(BodyType _type, const std::shared_ptr<Collider>& _collider, float _mass, float _friction)
        : m_type(_type), m_mass(_mass), m_friction(_friction), m_collider(_collider)
    {
        if (!_collider)
        {
            Error("[PHYSICS - RIGIDBODY] Tried initializing a RigidBody without a collider.");
            return;
        }

        // Calculate inertia if mass is more than 0 and body is dynamic
        btVec3 inertia {0.0f, 0.0f, 0.0f};
        if (m_type == BodyType::DYNAMIC && _mass > 0.0f)
        {
            if (m_collider->GetShape())
            {
                m_collider->GetShape()->calculateLocalInertia(static_cast<btScalar>(_mass), inertia);
            }
            else
            {
                Error("[PHYSICS - RIGIDBODY] Tried initializing a RigidBody without a shape.");
            }
        }

        btTransform transform;
        transform.setIdentity();

        btDefaultMotionState* motionState = new btDefaultMotionState(transform);

        // Constructs the rigidbody identity with the object's info
        btRigidBody::btRigidBodyConstructionInfo info(
            (m_type == BodyType::DYNAMIC) ? static_cast<btScalar>(_mass) : static_cast<btScalar>(0),
            motionState,
            m_collider->GetShape(),
            inertia
        );

        m_body = std::make_unique<btRigidBody>(info);
        m_body->setFriction(_friction);

        if (m_type == BodyType::KINEMATIC)
        {
            m_body->setCollisionFlags(m_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
            m_body->setActivationState(DISABLE_DEACTIVATION);
        }
    }

    RigidBody::~RigidBody()
    {
        if (m_addedToWorld)
        {
            Engine::GetInstance().GetPhysicsManager().RemoveRigidBody(this);
        }
    }

    BodyType RigidBody::GetType() const
    {
        return m_type;
    }

    btRigidBody* RigidBody::GetBody() const
    {
        return m_body.get();
    }

    void RigidBody::SetAddedToWorld(bool _added)
    {
        m_addedToWorld = _added;
    }

    bool RigidBody::IsAddedToWorld() const
    {
        return m_addedToWorld;
    }

    void RigidBody::SetPosition(const vec3& _pos)
    {
        if (!m_body)
        {
            Warn("[RIGIDBODY - SETTER] Tried updating RigidBody's position AFTER addition to physic scene");
            return;
        }

        auto& tr = m_body->getWorldTransform();
        tr.setOrigin(btVec3(
            static_cast<btScalar>(_pos.x),
            static_cast<btScalar>(_pos.y),
            static_cast<btScalar>(_pos.z))
            );

        if (m_body->getMotionState())
        {
            m_body->getMotionState()->setWorldTransform(tr);
        }
        m_body->setWorldTransform(tr);
    }

    vec3 RigidBody::GetPosition() const
    {
        const auto& pos = m_body->getWorldTransform().getOrigin();
        return vec3(pos.x(), pos.y(), pos.z());
    }

    void RigidBody::SetRotation(const quat& _rot)
    {
        if (!m_body)
        {
            Warn("[RIGIDBODY - SETTER] Tried updating RigidBody's rotation AFTER addition to physic scene");
            return;
        }

        auto& tr = m_body->getWorldTransform();
        tr.setRotation(btQuaternion(
            static_cast<btScalar>(_rot.x),
            static_cast<btScalar>(_rot.y),
            static_cast<btScalar>(_rot.z),
            static_cast<btScalar>(_rot.w)
            ));

        if (m_body->getMotionState())
        {
            m_body->getMotionState()->setWorldTransform(tr);
        }
        m_body->setWorldTransform(tr);
    }

    quat RigidBody::GetRotation() const
    {
        const auto& rot = m_body->getWorldTransform().getRotation();
        return quat(rot.w(), rot.x(), rot.y(), rot.z());
    }
}
