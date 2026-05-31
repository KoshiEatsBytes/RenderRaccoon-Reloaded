
#pragma once
#include "Types.h"

/**
 * Stores reusable data structs
 */
namespace RR
{
    /**
     * Global projection and view for a scene
     */
    struct CameraData
    {
        mat4 viewMatrix;
        mat4 projMatrix;
        vec3 position;
    };

    struct LightData
    {
        vec3 color;
        vec3 position;
    };
}