#pragma once

#include "Scene/Component.h"
#include "Physics/RigidBody.h"

namespace RR
{
    enum class PhysicsOwnership : uint8_t;

    class PhysicsComponent : public Component
    {
    public:
        COMPONENT(PhysicsComponent, Component)

        PhysicsComponent();
        PhysicsComponent(BodyType _type, float _mass = 0.0f, float _friction = 0.5f);
        ~PhysicsComponent() override;

        void Init() override;
        void Update(float _deltaTime) override;

        void Teleport(const vec3& _worldPos, bool _resetVelocity = true);
        void SetRotation(const quat& _worldRot, bool _resetVelocity = true);

        void ApplyForce(const vec3& _force, const vec3& _relativePos = vec3(0));
        void ApplyImpulse(const vec3& _impulse, const vec3& _relativePos = vec3(0));
        void ApplyCentralImpulse(const vec3& _direction);

        void SetDamping(float _linear, float _angular);
        void SetLinearLock(const vec3& _factor);
        void SetAngularLock(const vec3& _factor);

        // EXPENSIVE
        void SetScale(const vec3& _newScale);
        void Rebuild();

        int GetExecutionOrder() const override;
        std::weak_ptr<RigidBody> GetRigidBody() const;

        void SetLinearVelocity(const vec3& _vec);
        vec3 GetLinearVelocity() const;

        void SetAngularVelocity(const vec3& _vec);
        vec3 GetAngularVelocity() const;

        void SetParameters(BodyType _type, float _mass, float _friction);
        void SetType(BodyType _type);
        void SetMass(float _mass);
        void SetFriction(float _friction);

        BodyType GetType() const;
        float GetMass() const;
        float GetFriction() const;

    protected:
        void OnEnable() override;
        void OnDisable() override;

    private:
        static void AddCollidersRecursive(GameObject* _go,
                                          const mat4& _localToParent,
                                          btCompoundShape& _compound,
                                          std::vector<std::shared_ptr<Collider>>& colliders,
                                          PhysicsOwnership _ownership);

        static bool DecomposeTr(const mat4& _mat, vec3& _outPos, quat& _outRot, vec3& _outScale);

        static void ClearInheritedPhysicsOwnership(GameObject* _go);


        BodyType m_type = BodyType::STATIC;
        float m_mass = 0.0f;
        float m_friction = 0.5f;
        std::shared_ptr<RigidBody> m_rigidBody;
    };
}