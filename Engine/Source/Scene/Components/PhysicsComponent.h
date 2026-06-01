#pragma once

#include "Scene/Component.h"
#include "Physics/RigidBody.h"

namespace RR
{
    class PhysicsComponent : public Component
    {
    public:
        COMPONENT(PhysicsComponent)

        PhysicsComponent(BodyType _type, float _mass = 1.0f, float _friction = 0.5f);
        ~PhysicsComponent() override;

        void Init() override;
        void Update(float _deltaTime) override;

        int GetExecutionOrder() const override;

    private:
        static void AddCollidersRecursive(GameObject* _go, const mat4& _localToParent,
                                          btCompoundShape& _compound, std::vector<std::shared_ptr<Collider>>& colliders);

        static bool DecomposeTr(const mat4& _mat, vec3& _outPos, quat& _outRot, vec3& _outScale);


        BodyType m_type;
        float m_mass;
        float m_friction;
        std::shared_ptr<RigidBody> m_rigidBody;
    };
}