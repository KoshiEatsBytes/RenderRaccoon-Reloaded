
#pragma once
#include <memory>
#include <string>

#include "Helpers/Types.h"

namespace RR
{
    class Mesh;
    class Material;

    class VoxelSkybox
    {
    public:
        VoxelSkybox();
        ~VoxelSkybox();

        bool Load(const std::string& _skyMatPath, const std::string& _cloudMatPath);
        void BuildClouds(int _centreX, int _centreZ);

        // sky sets
        void SetHorizonColor(const vec3& _color);
        void SetZenithColor (const vec3& _color);
        void SetSun(const vec3& _direction, float _size);

        // cloud sets
        void SetClouds       (uInt32 _seed, int _height, float _scale, float _cov);
        void SetCloudSeed    (uInt32 _seed);
        void SetCloudHeight  (int _height);
        void SetCloudScale   (float _scale);
        void SetCloudCoverage(float _cov);
        void SetCloudColor   (const vec3& _color);
        void SetCloudFade    (float _fadeStart, float _fadeEnd);

        void SubmitSkyForDraw();
        void SubmitCloudsForDraw();

    private:
        std::shared_ptr<Mesh>     m_skyMesh;
        std::shared_ptr<Material> m_skyMat;

        // clouds
        std::shared_ptr<Mesh>     m_cloudMesh;
        std::shared_ptr<Material> m_cloudMat;

        int    m_cloudHeight   = 170;
        float  m_cloudScale    = 1.5f;
        float  m_cloudCoverage = 0.625f;
        uInt32 m_cloudSeed     = 42;
    };
}