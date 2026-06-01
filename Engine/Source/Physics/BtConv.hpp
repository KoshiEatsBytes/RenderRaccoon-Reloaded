
#pragma once
#include "Helpers/Types.h"
#include "LinearMath/btTransform.h"

namespace RR
{
    /**
     * Helper to implement easy conversion between bullet and glm data types :)
     */
    struct BtConv
    {
        static btVec3 ToBt(const vec3& _vec)
        {
            return btVec3{_vec.x, _vec.y, _vec.z};
        }

        static btQuat ToBt(const quat& _quat)
        {
            return btQuat{_quat.x, _quat.y, _quat.z, _quat.w};
        }

        static btTransform ToBtTransform(const vec3& _pos, const quat& _rot)
        {
            btTransform tr;
            tr.setOrigin(ToBt(_pos));
            tr.setRotation(ToBt(_rot));
            return tr;
        }

        static vec3 FromBt(const btVec3& _vec)
        {
            return vec3{_vec.x(), _vec.y(), _vec.z()};
        }

        static quat FromBt(const btQuat& _quat)
        {
            return quat{_quat.w(), _quat.x(), _quat.y(), _quat.z()};
        }
    };
}
