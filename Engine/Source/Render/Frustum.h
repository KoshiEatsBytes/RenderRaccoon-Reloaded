
#pragma once

#include "Helpers/Types.h"
#include <array>

namespace RR
{
    struct Frustum
    {
        using uInt8 = std::uint8_t;

        enum PLANE : uInt8
        {
            LEFT_PLANE,
            RIGHT_PLANE,
            BOTTOM_PLANE,
            TOP_PLANE,
            NEAR_PLANE,
            FAR_PLANE,

            COUNT
        };

        static Frustum FromViewProj(const mat4& _viewProj, bool _reversedZ = false)
        {
            Frustum ft;

            const vec4 row0(_viewProj[0][0], _viewProj[1][0], _viewProj[2][0], _viewProj[3][0]);
            const vec4 row1(_viewProj[0][1], _viewProj[1][1], _viewProj[2][1], _viewProj[3][1]);
            const vec4 row2(_viewProj[0][2], _viewProj[1][2], _viewProj[2][2], _viewProj[3][2]);
            const vec4 row3(_viewProj[0][3], _viewProj[1][3], _viewProj[2][3], _viewProj[3][3]);

            // Assembles the 6 camera planes
            ft.planes[LEFT_PLANE]   = row3 + row0;
            ft.planes[RIGHT_PLANE]  = row3 - row0;
            ft.planes[BOTTOM_PLANE] = row3 + row1;
            ft.planes[TOP_PLANE]    = row3 - row1;

            // Adjust for reverse Z
            if (_reversedZ)
            {
                ft.planes[NEAR_PLANE] = row3 - row2;
                ft.planes[FAR_PLANE]  = row2;
            }
            else
            {
                ft.planes[NEAR_PLANE] = row3 + row2;
                ft.planes[FAR_PLANE]  = row3 - row2;
            }

            // Normalize so distances are metric
            for (vec4& plane : ft.planes)
            {
                plane /= glm::length(vec3(plane));
            }

            return ft;
        }

        // Camera projection AABB check
        bool IntersectsAABB(const vec3& _min, const vec3& _max) const
        {
            for (const vec4& plane : planes)
            {
                const vec3 norm (plane);

                // Corner of aabb which is most towards plane normal
                const vec3 posVert {
                    norm.x >= 0.0f ? _max.x : _min.x,
                    norm.y >= 0.0f ? _max.y : _min.y,
                    norm.z >= 0.0f ? _max.z : _min.z
                };

                // if the most favourable corner is outside the plane, everything is outside
                if (glm::dot(norm, posVert) + plane.w < 0.0f)
                {
                    return false;
                }
            }

            return true;
        }

        std::array<vec4, static_cast<uInt8>(COUNT)> planes;
    };
}