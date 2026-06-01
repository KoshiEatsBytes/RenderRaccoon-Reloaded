
#pragma once
#include <memory>

#include "Helpers/Types.h"
#include "Collider.h"
#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>

class btRigidBody;
class btCompoundShape;
struct btDefaultMotionState;

namespace RR
{
    class GameObject;
    /**
     * Three main rigidbody behaviors
     *
     *  STATIC: Immovable object, for collisions with surroundings
     *  DYNAMIC: Fully simulated physics object, affected by gravity
     *  KINEMATIC: Simulated physics controlled manually
     */
    enum class BodyType
    {
        STATIC,
        DYNAMIC,
        KINEMATIC
    };

    class RigidBody
    {
    public:
        RigidBody(
            std::unique_ptr<btCompoundShape> _compound,
            std::vector<std::shared_ptr<Collider>> _colliders,
            BodyType _type, float _mass, float _friction);
        ~RigidBody();

        BodyType GetType() const;
        btRigidBody* GetBody() const;
        btDefaultMotionState* GetMotionState() const;

        void SetAddedToWorld(bool _added);
        bool IsAddedToWorld() const;

        void SetMask(int _mask);
        int GetMask() const;
        int GetGroup() const;

        void SetPosition(const vec3& _pos, bool _reset = false);
        vec3 GetPosition() const;

        void SetRotation(const quat& _rot, bool _reset = false);
        quat GetRotation() const;

    private:
        BodyType m_type = BodyType::STATIC;
        bool m_addedToWorld = false;
        float m_mass = 0.0f;
        float m_friction = 0.5f;

        int m_group = btBroadphaseProxy::StaticFilter;
        int m_mask = btBroadphaseProxy::AllFilter;

        std::vector<std::shared_ptr<Collider>> m_colliders;
        std::unique_ptr<btCompoundShape> m_compound;
        std::unique_ptr<btDefaultMotionState> m_motionState;
        std::unique_ptr<btRigidBody> m_body;
    };
}
