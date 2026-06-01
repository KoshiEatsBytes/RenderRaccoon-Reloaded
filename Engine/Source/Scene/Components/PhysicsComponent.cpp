
#include "PhysicsComponent.h"

#include "ColliderComponent.h"
#include "Engine.h"
#include "LinearMath/btDefaultMotionState.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "Helpers/Printer.hpp"
#include "Physics/BtConv.hpp"
#include "Scene/GameObject.h"
#include "Physics/RigidBody.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    PhysicsComponent::PhysicsComponent(BodyType _type, float _mass, float _friction)
        : m_type(_type), m_mass(_mass), m_friction(_friction)
    {
    }

    PhysicsComponent::~PhysicsComponent()
    = default;

    void PhysicsComponent::Init()
    {
        // Detect two physic components, thats no good!
        auto siblings = m_owner->GetComponents<PhysicsComponent>();
        if (siblings.size() > 1)
        {
            Error("[PHYSICS COMPONENT] Multiple PhysicsComponents on '",
                  m_owner->GetName(), "' — only one is supported per GameObject");
            return;
        }

        // Build compound shape for this GO with its colliders
        auto compound = std::make_unique<btCompoundShape>();
        std::vector<std::shared_ptr<Collider>> colliders;

        // Goes through every child and checks if they have components, carry scale
        // UNIFORM WORLD SCALE ONLY!!!!
        mat4 initial = glm::scale(mat4(1.0f), m_owner->GetWorldScale());
        AddCollidersRecursive(m_owner, initial, *compound, colliders);

        if (compound->getNumChildShapes() == 0)
        {
            Warn("[PHYSICS COMPONENT] Game-Object '", m_owner->GetName(),
                "' Has a physics component but NO colliders in heirarchy");
            return;
        }

        // Build rigidbody with the compound
        m_rigidBody = std::make_shared<RigidBody>(
            std::move(compound),
            std::move(colliders),
            m_type,
            m_mass,
            m_friction
            );


        // Push world pos into body
        m_rigidBody->SetPosition(m_owner->GetWorldPosition());
        m_rigidBody->SetRotation(m_owner->GetWorldRotation());
        Engine::GetInstance().GetPhysicsManager().AddRigidbody(m_rigidBody.get());
    }

    void PhysicsComponent::Update(float _deltaTime)
    {
        if (!m_rigidBody)
        {
            return;
        }

        // handle physic body respectively to its type
        switch (m_rigidBody->GetType())
        {
            case BodyType::STATIC:
            {
                // Static bodies don't move!
            }
            break;

            case BodyType::DYNAMIC:
            {
                // Physics to GO (world to local)
                m_owner->SetWorldPosition(m_rigidBody->GetPosition());
                m_owner->SetWorldRotation(m_rigidBody->GetRotation());
            }
            break;

            case BodyType::KINEMATIC:
            {
                // GO -> Physics
                // For bullet kinematics, drive motion state
                btTransform tr = BtConv::ToBtTransform(
                    m_owner->GetWorldPosition(),
                    m_owner->GetWorldRotation()
                    );

                if (auto motionState = m_rigidBody->GetMotionState())
                {
                    motionState->setWorldTransform(tr);
                }
            }
            break;
        }
    }

    int PhysicsComponent::GetExecutionOrder() const
    {
        return -100;
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    void PhysicsComponent::AddCollidersRecursive(GameObject *_go, const mat4& _localToParent, btCompoundShape &_compound,
        std::vector<std::shared_ptr<Collider>>& colliders)
    {
        // add this current GO's collider if it has any
        // cc -> Collider Component
        auto components = _go->GetComponents<ColliderComponent>();
        if (!components.empty())
        {
            vec3 pos, scale;
            quat rot;

            if (DecomposeTr(_localToParent, pos, rot, scale))
            {
                for (auto* cc : components)
                {
                    auto shape = cc->GetCollider()->GetShape();
                    shape->setLocalScaling(BtConv::ToBt(scale));
                    _compound.addChildShape(BtConv::ToBtTransform(pos, rot), shape);
                    colliders.push_back(cc->GetColliderShared());
                }
            }
            else
            {
                Warn("[PHYSICS] Skipping colliders on '", _go->GetName(),
                     "' due to unsupported transform");
            }
        }

        // recurse until sub-bodies
        for (auto& child : _go->GetChildren())
        {
            if (!child->IsActive()) continue;
            if (child->GetComponent<PhysicsComponent>()) continue;

            mat4 childLocal = _localToParent * child->GetLocalTransform();
            AddCollidersRecursive(child.get(), childLocal, _compound, colliders);
        }
    }

    bool PhysicsComponent::DecomposeTr(const mat4 &_mat, vec3 &_outPos, quat &_outRot, vec3 &_outScale)
    {
        vec3 skew;
        vec4 perspective;

        if (!glm::decompose(_mat, _outScale, _outRot, _outPos, skew, perspective))
        {
            return false;
        }

        // in case of shear
        constexpr float kSkewEpsilon = 1e-5f;
        if (glm::length(skew) > kSkewEpsilon)
        {
            Warn("[PHYSICS] Sheared transform in physics hierarchy — unsupported");
            return false;
        }

        return true;
    }
}
