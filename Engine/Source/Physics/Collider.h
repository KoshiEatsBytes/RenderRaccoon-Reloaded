
#pragma once
#include <memory>

#include "Helpers/Types.h"

class btCollisionShape;

namespace RR
{
    class Collider
    {
    public:
        Collider();
        virtual ~Collider();

        btCollisionShape* GetShape() const;

    protected:
        std::unique_ptr<btCollisionShape> m_shape;
    };

    // Derived colliders -----------------------------------------------------------------------------------------------

    class BoxCollider : public Collider
    {
    public:
        explicit BoxCollider(const vec3& _extents);
    };

    class SphereCollider : public Collider
    {
    public:
        explicit SphereCollider(float _radius);
    };

    class CapsuleCollider : public Collider
    {
    public:
        explicit CapsuleCollider(float _radius, float _height);
    };
}
