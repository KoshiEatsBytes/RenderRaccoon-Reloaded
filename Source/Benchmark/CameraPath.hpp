
#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

#include "Helpers/Types.h"

namespace BENCH
{
    using uInt8 = std::uint8_t;

    struct PathSegment
    {
        enum class MOVE
        {
            GOTO,
            HOLD
        };

        enum class LOOK
        {
            FACE_TRAVEL,
            ROTATE_YAW,
            FIXED
        };

        // Movement
        MOVE  move        = MOVE::GOTO;
        vec3  target      { 0.0f };
        float speed       = 20.f;
        float holdSeconds = 0.0f;

        // Rotation
        LOOK  look        = LOOK::FACE_TRAVEL;
        float yawSweepDeg = 0.0f;
        float fixedYaw    = 0.0f;
        float fixedPitch  = 0.0f;

        // turn in blend duration
        float turnInSeconds = 2.0f;
    };

    struct CameraSample
    {
        vec3  position {0.0f };
        float yaw      = 0.0f;
        float pitch    = 0.0f;
    };

    class CameraPath
    {
    public:
        CameraPath() = default;
        ~CameraPath() = default;

        void Begin(const vec3& _startPos, float _startYaw, float _startPitch)
        {
            m_startPos   = _startPos;
            m_startYaw   = _startYaw;
            m_startPitch = _startPitch;

            // Reset existing
            m_legs.clear();
            m_total = 0.0f;
        }

        void Add(const PathSegment& _seg)
        {
            Baked bk;
            bk.segment = _seg;

            // chain onto prev leg
            if (m_legs.empty())
            {
                bk.from    = m_startPos;
                bk.yawIn   = m_startYaw;
                bk.pitchIn = m_startPitch;
            }
            else
            {
                // resume from prev leg
                bk.from    = m_legs.back().to;
                bk.yawIn   = m_legs.back().yawOut;
                bk.pitchIn = m_legs.back().pitchIn;
            }

            // Either move cam or hold
            if (_seg.move == PathSegment::MOVE::GOTO) {
                bk.to = _seg.target;
            }
            else {
                bk.to = bk.from;
            }

            const float dist = glm::length(bk.to - bk.from);

            // goto uses moves, otherwise hold
            if (_seg.move == PathSegment::MOVE::GOTO)
            {
                bk.duration = dist / std::max(_seg.speed, 0.0001f);
            }
            else
            {
                bk.duration = _seg.holdSeconds;
            }

            // yaw the leg ends so next leg starts continuos
            bk.yawOut = ResolveEndYaw(bk);
            bk.tStart = m_total;

            // add up duration and push
            m_total   += bk.duration;
            m_legs.push_back(bk);
        }

        float Duration() const
        {
            return m_total;
        }

        CameraSample Sample(float _t) const
        {
            // default to start
            CameraSample sample {
                m_startPos,
                m_startYaw,
                m_startPitch
            };

            // no legs, return start
            if (m_legs.empty()) return sample;

            // clamp step
            _t = std::clamp(_t, 0.0f, m_total);

            for (const Baked& baked : m_legs)
            {
                // skip this leg if t is past its end its not last leg
                if (_t > baked.tStart + baked.duration &&
                    &baked != &m_legs.back())
                    continue;

                const bool  durNot0    = baked.duration > 0.0f;
                const bool  fixedPitch = baked.segment.look == PathSegment::LOOK::FIXED;

                // normalize t to a local from o to 1
                const float local      = durNot0 ? (_t - baked.tStart) / baked.duration : 1.0f;

                // Interpolate position between leg start and end
                sample.position = glm::mix(baked.from, baked.to, std::clamp(local, 0.0f, 1.0f));

                // pitch is either fixed or interpolated, yaw calcualted outside
                sample.pitch    = fixedPitch ? baked.segment.fixedPitch : baked.pitchIn;
                sample.yaw      = SampleYaw(baked, local);

                return sample;
            }

            return sample;
        }

    private:
        // one saved camera step
        struct Baked
        {
            PathSegment segment;

            vec3  from     {0.0f};
            vec3  to       {0.0f};
            float yawIn    {};
            float yawOut   {};
            float pitchIn  {};
            float duration {};
            float tStart   {};
        };

        // yaw to face travel direction
        static float FaceTravelYaw(const vec3& _from, const vec3& _to, float _fallback)
        {
            const vec3 distance = _to - _from;
            if (glm::dot(distance, distance) < 1e-8f) return _fallback;
            return glm::degrees(std::atan2(-distance.x, -distance.z));
        }

        // interpolation between two degree angles
        static float LerpAngleDeg(float _a, float _b, float _t)
        {
            const float diff = std::fmod(_b - _a + 540.0f, 360.0f) - 180.0f;
            return _a + diff * std::clamp(_t, 0.0f, 1.0f);
        }

        static float ResolveEndYaw(const Baked& _baked)
        {
            switch (_baked.segment.look)
            {
                case PathSegment::LOOK::FACE_TRAVEL:
                    return FaceTravelYaw(_baked.from, _baked.to, _baked.yawIn);

                case PathSegment::LOOK::ROTATE_YAW:
                    return _baked.yawIn + _baked.segment.yawSweepDeg;

                case PathSegment::LOOK::FIXED:
                    return _baked.segment.fixedYaw;
            }
            return _baked.yawIn;
        }

        static float SampleYaw(const Baked& _baked, float _local)
        {
            switch (_baked.segment.look) {
                case PathSegment::LOOK::FACE_TRAVEL:
                {
                    const float travel = FaceTravelYaw(_baked.from, _baked.to, _baked.yawIn);
                    const float turn   = std::min(_baked.segment.turnInSeconds, _baked.duration);
                    const float secs   = _local * _baked.duration;
                    const float t      = turn > 0.0f ? secs / turn : 1.0f;

                    return LerpAngleDeg(_baked.yawIn, travel, t);
                }

                case PathSegment::LOOK::ROTATE_YAW:
                    return _baked.yawIn + _baked.segment.yawSweepDeg * _local;

                case PathSegment::LOOK::FIXED:
                    return _baked.segment.fixedYaw;
            }
            return _baked.yawIn;
        }

        std::vector<Baked> m_legs;

        // Start vals
        vec3  m_startPos   {0.0f};
        float m_startYaw   = 0.0f;
        float m_startPitch = 0.0f;
        float m_total      = 0.0f;
    };

    enum class CAMERA_PATH_ID : uInt8
    {
        DETERMINISTIC,
        COUNT
    };

    // Helper to retrieve the camera path for a scenario
    inline CameraPath GetCameraPath(CAMERA_PATH_ID _id = CAMERA_PATH_ID::DETERMINISTIC)
    {
        CameraPath path;

        switch (_id)
        {
            case CAMERA_PATH_ID::DETERMINISTIC:
            {
                const vec3 spawn(0.f, 170.f, 0.f);
                path.Begin(spawn, 0.f, -20.f);

                PathSegment l1;
                l1.move = PathSegment::MOVE::GOTO;
                l1.target = vec3(600.f, 160.f, -600.f);
                l1.speed = 45.f;
                l1.look  = PathSegment::LOOK::FACE_TRAVEL;
                path.Add(l1);

                PathSegment look1;
                look1.move = PathSegment::MOVE::HOLD;
                look1.holdSeconds = 6.f;
                look1.look = PathSegment::LOOK::ROTATE_YAW;
                look1.yawSweepDeg = 360.f;
                path.Add(look1);

                PathSegment back;
                back.move = PathSegment::MOVE::GOTO;
                back.target = spawn;
                back.speed = 45.f;
                back.look  = PathSegment::LOOK::FACE_TRAVEL;
                path.Add(back);
            }
            break;

            default:
                RR::Error("[CAMERA PATH] Warning, invalid camera path requested!");
            break;
        }

        return path;
    }
}









































