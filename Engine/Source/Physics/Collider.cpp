
#include "Collider.h"

#include <btBulletCollisionCommon.h>

namespace RR
{
    // BASE COLLIDER ---------------------------------------------------------------------------------------------------

    Collider::Collider()
    = default;

    Collider::~Collider()
    = default;

    btCollisionShape* Collider::GetShape() const
    {
        return m_shape.get();
    }

    // BOX COLLIDER ----------------------------------------------------------------------------------------------------

    BoxCollider::BoxCollider(const vec3& _extents)
    {
        vec3 halfExtents = _extents * 0.5f;
        m_shape = std::make_unique<btBoxShape>(btVec3(halfExtents.x, halfExtents.y, halfExtents.z));
    }

    // SPHERE COLLIDER -------------------------------------------------------------------------------------------------

    SphereCollider::SphereCollider(const float _radius)
    {
        m_shape = std::make_unique<btSphereShape>(_radius);
    }

    // CAPSULE COLLIDER ------------------------------------------------------------------------------------------------

    CapsuleCollider::CapsuleCollider(float _radius, float _height)
    {
        m_shape = std::make_unique<btCapsuleShape>(static_cast<btScalar>(_radius), static_cast<btScalar>(_height));
    }
}
