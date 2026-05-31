
#pragma once

#include "../../Helpers/Types.h"

namespace RR
{
    /**
     * Key frame for animation for vec3
     */
    struct KeyFrameVec3
    {
        float timeStamp = 0.0f;
        vec3 value = vec3(0.0f);
    };

    /**
     * Key frame for animations fo Quat
     */
    struct KeyFrameQuat
    {
        float timeStamp = 0.0f;
        quat value = quat(1.0f, 0.0f, 0.0f, 0.0f);
    };

    /**
     * Animation track containing keyframes
     */
    struct TransformTrack
    {
        std::string targetName;
        std::vector<KeyFrameVec3> positions;
        std::vector<KeyFrameQuat> rotations;
        std::vector<KeyFrameVec3> scales;
    };

    /**
     * Contains all data for an animation
     */
    struct AnimationClip
    {
        std::string name;
        float duration = 0.0f;
        bool looping = true;
        std::vector<TransformTrack> tracks;
    };

    struct ObjectBinding
    {
        GameObject* object = nullptr;
        std::vector<sizeT> trackIndices;
    };
}