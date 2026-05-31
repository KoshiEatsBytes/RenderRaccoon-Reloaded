
#pragma once
#include <memory>

#include "Helpers/Types.h"
#include "Collider.h"

class btRigidBody;

namespace RR
{
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
        RigidBody(BodyType _type, const std::shared_ptr<Collider>& _collider,
            float _mass, float _friction);
        ~RigidBody();

        BodyType GetType() const;
        btRigidBody* GetBody() const;

        void SetAddedToWorld(bool _added);
        bool IsAddedToWorld() const;

        void SetPosition(const vec3& _pos);
        vec3 GetPosition() const;

        void SetRotation(const quat& _rot);
        quat GetRotation() const;

    private:
        BodyType m_type = BodyType::STATIC;
        bool m_addedToWorld = false;
        float m_mass = 0.0f;
        float m_friction = 0.5f;

        std::unique_ptr<btRigidBody> m_body;
        std::shared_ptr<Collider> m_collider;
    };
}
