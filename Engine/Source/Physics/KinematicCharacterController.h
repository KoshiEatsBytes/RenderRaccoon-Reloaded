
#pragma once
#include <memory>

#include "Helpers/Types.h"

class btPairCachingGhostObject;
class btKinematicCharacterController;
class btCapsuleShape;
class btGhostPairCallback;

namespace RR
{
    class KinematicCharacterController
    {
    public:
        KinematicCharacterController(float _radius, float _height);
        ~KinematicCharacterController();

        void SetWalkVelocity(const vec3& _velocity, float _duration);
        void Jump(const vec3& _direction);
        bool OnGround() const;

        void Resize(float _radius, float _height);

        void SetPosition(const vec3& _pos);
        vec3 GetPosition() const;

    private:
        void BuildGhostAndController(const vec3& _startPos);
        void TearDown();

        std::unique_ptr<btCapsuleShape> m_capsule;
        std::unique_ptr<btPairCachingGhostObject> m_ghost;
        std::unique_ptr<btKinematicCharacterController> m_controller;

        float m_height = 1.2f;
        float m_radius = 0.4f;
        float m_cameraOffset = 0.5f;

        // max step height and slop climb for Character Controller
        btScalar m_stepHeight = 0.35f;
        btScalar m_maxSlope = 50.0f;
    };
}

