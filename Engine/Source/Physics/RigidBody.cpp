
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

        if (m_type == BodyType::DYNAMIC && _mass <= 0.0f)
        {
            Error("[PHYSICS - RIGIDBODY] DYNAMIC body created with zero or negative mass.");
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
                return;
            }
        }

        btTransform transform;
        transform.setIdentity();

        m_motionState = std::make_unique<btDefaultMotionState>(transform);

        // Constructs the rigidbody identity with the object's info
        btRigidBody::btRigidBodyConstructionInfo info(
            (m_type == BodyType::DYNAMIC) ? static_cast<btScalar>(_mass) : static_cast<btScalar>(0),
            m_motionState.get(),
            m_collider->GetShape(),
            inertia
        );

        m_body = std::make_unique<btRigidBody>(info);
        m_body->setFriction(_friction);

        switch (m_type)
            {
            case BodyType::STATIC:
                m_group = btBroadphaseProxy::StaticFilter;
                m_mask = btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::CharacterFilter | btBroadphaseProxy::KinematicFilter;
                break;

            case BodyType::DYNAMIC:
                m_group = btBroadphaseProxy::DefaultFilter;
                m_mask = btBroadphaseProxy::AllFilter;
                break;

            case BodyType::KINEMATIC:
                m_group = btBroadphaseProxy::KinematicFilter;
                m_mask = btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::CharacterFilter | btBroadphaseProxy::StaticFilter;
                m_body->setCollisionFlags(m_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
                m_body->setActivationState(DISABLE_DEACTIVATION);
                break;
        }
    }

    RigidBody::~RigidBody()
    {
        if (m_addedToWorld)
        {
            Engine::GetInstance().GetPhysicsManager().RemoveRigidBody(this);
        }

        m_body.reset();
        m_motionState.reset();
        m_collider.reset();
    }

    BodyType RigidBody::GetType() const
    {
        return m_type;
    }

    btRigidBody* RigidBody::GetBody() const
    {
        return m_body.get();
    }

    btDefaultMotionState* RigidBody::GetMotionState() const
    {
        return m_motionState.get();
    }

    void RigidBody::SetAddedToWorld(bool _added)
    {
        m_addedToWorld = _added;
    }

    bool RigidBody::IsAddedToWorld() const
    {
        return m_addedToWorld;
    }

    void RigidBody::SetMask(const int _mask)
    {
        if (m_addedToWorld)
        {
            Warn("[RIGIDBODY - MASK] Tried updating mask layer after adding to physics scene");
            return;
        }

        m_mask = _mask;
    }

    int RigidBody::GetMask() const
    {
        return m_mask;
    }

    int RigidBody::GetGroup() const
    {
        return m_group;
    }

    void RigidBody::SetPosition(const vec3& _pos, bool _reset)
    {
        if (!m_body)
        {
            Warn("[RIGIDBODY - SETTER] Tried updating RigidBody's position AFTER constructor failed");
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

        if (_reset)
        {
            m_body->setLinearVelocity({0.f,0.f,0.f});
            m_body->setAngularVelocity({0.f,0.f,0.f});
        }
        m_body->activate(true);
    }

    vec3 RigidBody::GetPosition() const
    {
        const auto& pos = m_body->getWorldTransform().getOrigin();
        return vec3(pos.x(), pos.y(), pos.z());
    }

    void RigidBody::SetRotation(const quat& _rot, bool _reset)
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

        if (_reset)
        {
            m_body->setLinearVelocity({0.f,0.f,0.f});
            m_body->setAngularVelocity({0.f,0.f,0.f});
        }
        m_body->activate(true);
    }

    quat RigidBody::GetRotation() const
    {
        const auto& rot = m_body->getWorldTransform().getRotation();
        return quat(rot.w(), rot.x(), rot.y(), rot.z());
    }
}
