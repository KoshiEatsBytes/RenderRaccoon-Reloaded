
#pragma once
#include <memory>

#include "Helpers/Types.h"

class btPairCachingGhostObject;
class btKinematicCharacterController;
class btCapsuleShape;
class btGhostPairCallback;

namespace RR
{
    class PhysicsManager;
    class KinematicCharacterController
    {
    public:
        KinematicCharacterController(PhysicsManager& _pm, float _radius, float _height);
        ~KinematicCharacterController();

        void SetWalkVelocity(const vec3& _velocity) const;
        void Jump(const vec3& _direction);
        bool OnGround() const;

        void Resize(float _radius, float _height);
        void AddToWorld();
        void RemoveFromWorld();

        void SetPosition(const vec3& _pos);
        vec3 GetPosition() const;

        void RecordPositionBeforeStep();
        void RecordPositionAfterStep();
        vec3 GetInterpolatedPosition(float _alpha) const;

    private:
        void BuildGhostAndController(const vec3& _startPos);

        PhysicsManager& m_physicsManager;
        std::unique_ptr<btCapsuleShape> m_capsule;
        std::unique_ptr<btPairCachingGhostObject> m_ghost;
        std::unique_ptr<btKinematicCharacterController> m_controller;

        bool m_inWorld = false;
        float m_height = 1.2f;
        float m_radius = 0.4f;
        float m_cameraOffset = 0.5f;

        // max step height and slop climb for Character Controller
        btScalar m_stepHeight = 0.35f;
        btScalar m_maxSlope = 50.0f;

        // For camera interpolation (fps 60+)
        vec3 m_prevGhostPos = vec3(0.0f);
        vec3 m_currGhostPos = vec3(0.0f);

        // For stepcallbacks register
        sizeT m_preStepHandle  = 0;
        sizeT m_postStepHandle = 0;
    };
}

